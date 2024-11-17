/*
# Copyright 2022 University of California, Riverside
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# SPDX-License-Identifier: Apache-2.0
*/

#include <errno.h>
#include <math.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <time.h>
#include <unistd.h>

#include <rte_branch_prediction.h>
#include <rte_eal.h>
#include <rte_errno.h>
#include <rte_memzone.h>

#include "http.h"
#include "io.h"
#include "log.h"
#include "spright.h"
#include "utility.h"

// sockfd_*[i] = [server, client]
static int sockfd_rx[UINT8_MAX][2];  // Pairs of connected sockets for communication between nf_rx and nf_worker
static int sockfd_tx[UINT8_MAX][2];  // Pairs of connected sockets for communication between nf_worker and nf_tx

// Helper function to create and connect socket pairs
static int create_socket_pair(int sockets[2]) {
    struct sockaddr_in serv_addr;
    socklen_t addr_len = sizeof(serv_addr);
    int server_fd, client_fd;
    int optval = 1;
    
    // Clear the address structure first
    memset(&serv_addr, 0, sizeof(serv_addr));
    
    // Create server socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (unlikely(server_fd == -1)) {
        log_error("socket() error: %s", strerror(errno));
        return -1;
    }

    // Set socket options
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int)) == -1) {
        log_error("setsockopt() error: %s", strerror(errno));
        close(server_fd);
        return -1;
    }

    // Setup server address
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(0);
    serv_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    
    // Attempt binding
    if (bind(server_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1) {
        log_error("bind() error: %s (errno: %d)", strerror(errno), errno);
        close(server_fd);
        return -1;
    }

    // Get the assigned port number
    if (getsockname(server_fd, (struct sockaddr *)&serv_addr, &addr_len) == -1) {
        log_error("getsockname() error: %s", strerror(errno));
        close(server_fd);
        return -1;
    }

    // Start server listening
    if (listen(server_fd, 1) == -1) {
        log_error("listen() error: %s", strerror(errno));
        close(server_fd);
        return -1;
    }

    // Create client socket
    client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (unlikely(client_fd == -1)) {
        log_error("client socket() error: %s", strerror(errno));
        close(server_fd);
        return -1;
    }

    // Connect client socket to server
    if (connect(client_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1) {
        log_error("connect() error: %s", strerror(errno));
        close(server_fd);
        close(client_fd);
        return -1;
    }

    sockets[0] = accept(server_fd, NULL, NULL);
    sockets[1] = client_fd;

    if (unlikely(sockets[0] == -1)) {
        log_error("accept() error: %s", strerror(errno));
        close(server_fd);
        close(client_fd);
        return -1;
    }
    
    close(server_fd);
    
    return 0;
}

static int autoscale_memory(uint8_t mb)
{
    char *buffer = NULL;

    if (unlikely(mb == 0))
    {
        return 0;
    }

    buffer = malloc(1000000 * mb * sizeof(char));
    if (unlikely(buffer == NULL))
    {
        log_error("malloc() error: %s", strerror(errno));
        return -1;
    }

    buffer[0] = 'a';
    buffer[1000000 * mb - 1] = 'a';

    free(buffer);

    return 0;
}

static int autoscale_sleep(uint32_t ns)
{
    struct timespec interval;
    int ret;

    interval.tv_sec = ns / 1000000000;
    interval.tv_nsec = ns % 1000000000;

    ret = nanosleep(&interval, NULL);
    if (unlikely(ret == -1))
    {
        log_error("nanosleep() error: %s", rte_strerror(errno));
        return -1;
    }

    return 0;
}

static int autoscale_compute(uint32_t n)
{
    uint32_t i;

    for (i = 2; i < sqrt(n); i++)
    {
        if (n % i == 0)
        {
            break;
        }
    }

    return 0;
}

static void *nf_worker(void *arg)
{
    struct http_transaction *txn = NULL;
    uint8_t index;

    /* TODO: Careful with this pointer as it may point to a stack */
    index = (uint64_t)arg;

    log_debug("Initialized NF %u worker %u", fn_id, index);

    while (1)
    {
        // Wait and read incoming transaction
        if (unlikely(recv(sockfd_rx[index][0], &txn, sizeof(struct http_transaction *), 0) == -1))
        {
            log_error("read() error: %s", strerror(errno));
            return NULL;
        }

        log_debug("Fn#%d is processing request.", fn_id);

        if (unlikely(autoscale_memory(cfg->nf[fn_id - 1].param.memory_mb) == -1))
        {
            log_error("autoscale_memory() error");
            return NULL;
        }

        if (unlikely(autoscale_sleep(cfg->nf[fn_id - 1].param.sleep_ns) == -1))
        {
            log_error("autoscale_sleep() error");
            return NULL;
        }

        if (unlikely(autoscale_compute(cfg->nf[fn_id - 1].param.compute) == -1))
        {
            log_error("autoscale_compute() error");
            return NULL;
        }

        // Write outgoing transaction
        if (unlikely(send(sockfd_tx[index][1], &txn, sizeof(struct http_transaction *), 0) == -1))
        {
            log_error("write() error: %s", strerror(errno));
            return NULL;
        }
    }

    return NULL;
}

static void *nf_rx(void *arg)
{
    struct http_transaction *txn = NULL;

    log_debug("Initialized NF %u nf_rx", fn_id);

    for (uint8_t i = 0;; i = (i + 1) % cfg->nf[fn_id - 1].n_threads)
    {
        if (unlikely(io_rx((void **)&txn) == -1))
        {
            log_error("io_rx() error");
            return NULL;
        }

        log_debug("RX processing txn");

        debug_http_transaction(txn);

        if (unlikely(send(sockfd_rx[i][1], &txn, sizeof(struct http_transaction *), 0) == -1))
        {
            log_error("write() error: %s", strerror(errno));
            return NULL;
        }
    }

    return NULL;
}

static void *nf_tx(void *arg)
{
    struct epoll_event event[UINT8_MAX]; /* TODO: Use Macro */
    struct http_transaction *txn = NULL;
    int n_fds;
    int epfd;

    epfd = epoll_create1(0);
    if (unlikely(epfd == -1))
    {
        log_error("epoll_create1() error: %s", strerror(errno));
        return NULL;
    }

    for (uint8_t i = 0; i < cfg->nf[fn_id - 1].n_threads; i++) {
        if (unlikely(set_nonblocking(sockfd_tx[i][0]) == -1)) {
            return NULL;
        }

        event[0].events = EPOLLIN;
        event[0].data.fd = sockfd_tx[i][0];

        if (unlikely(epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd_tx[i][0], &event[0]) == -1)) {
            log_error("epoll_ctl() error: %s", strerror(errno));
            return NULL;
        }
    }

    log_debug("Initialized NF %u nf_tx", fn_id);

    while (1)
    {
        n_fds = epoll_wait(epfd, event, cfg->nf[fn_id - 1].n_threads, -1);
        if (unlikely(n_fds == -1))
        {
            log_error("epoll_wait() error: %s", strerror(errno));
            return NULL;
        }

        log_debug("NF %u received %u event notifications", fn_id, n_fds);

        for (uint8_t i = 0; i < n_fds; i++)
        {
            if (unlikely(read(event[i].data.fd, &txn, sizeof(struct http_transaction *)) == -1))
            {
                log_error("read() error: %s", strerror(errno));
                return NULL;
            }

            log_debug("TX processing txn");

            txn->hop_count++;

            if (likely(txn->hop_count < cfg->route[txn->route_id].length))
            {
                txn->next_fn = cfg->route[txn->route_id].hop[txn->hop_count];
            }
            else
            {
                txn->next_fn = 0;
            }

            debug_http_transaction(txn);

            if (unlikely(io_tx(txn, txn->next_fn) == -1))
            {
                log_error("io_tx() error");
                return NULL;
            }
        }
    }

    return NULL;
}

/* TODO: Cleanup on errors */
static int nf(uint8_t nf_id)
{
    const struct rte_memzone *memzone = NULL;
    pthread_t thread_worker[UINT8_MAX];
    pthread_t thread_rx;
    pthread_t thread_tx;
    uint8_t i;
    int ret;

    fn_id = nf_id;

    memzone = rte_memzone_lookup(MEMZONE_NAME);
    if (unlikely(memzone == NULL))
    {
        log_error("rte_memzone_lookup() error");
        return -1;
    }

    cfg = memzone->addr;

    ret = io_init();
    if (unlikely(ret == -1))
    {
        log_error("io_init() error");
        return -1;
    }

    for (i = 0; i < cfg->nf[fn_id - 1].n_threads; i++) {
        ret = create_socket_pair(sockfd_rx[i]);
        if (unlikely(ret == -1)) {
            log_error("create_socket_pair() error for rx");
            return -1;
        }

        ret = create_socket_pair(sockfd_tx[i]);
        if (unlikely(ret == -1)) {
            log_error("create_socket_pair() error for tx");
            return -1;
        }
    }

    ret = pthread_create(&thread_rx, NULL, &nf_rx, NULL);
    if (unlikely(ret != 0))
    {
        log_error("pthread_create() error: %s", strerror(ret));
        return -1;
    }

    ret = pthread_create(&thread_tx, NULL, &nf_tx, NULL);
    if (unlikely(ret != 0))
    {
        log_error("pthread_create() error: %s", strerror(ret));
        return -1;
    }

    for (i = 0; i < cfg->nf[fn_id - 1].n_threads; i++)
    {
        ret = pthread_create(&thread_worker[i], NULL, &nf_worker, (void *)(uint64_t)i);
        if (unlikely(ret != 0))
        {
            log_error("pthread_create() error: %s", strerror(ret));
            return -1;
        }
    }

    for (i = 0; i < cfg->nf[fn_id - 1].n_threads; i++)
    {
        ret = pthread_join(thread_worker[i], NULL);
        if (unlikely(ret != 0))
        {
            log_error("pthread_join() error: %s", strerror(ret));
            return -1;
        }
    }

    ret = pthread_join(thread_rx, NULL);
    if (unlikely(ret != 0))
    {
        log_error("pthread_join() error: %s", strerror(ret));
        return -1;
    }

    ret = pthread_join(thread_tx, NULL);
    if (unlikely(ret != 0))
    {
        log_error("pthread_join() error: %s", strerror(ret));
        return -1;
    }

    for (i = 0; i < cfg->nf[fn_id - 1].n_threads; i++) {
        ret = close(sockfd_rx[i][0]);
        if (unlikely(ret == -1)) {
            log_error("close() error: %s", strerror(errno));
            return -1;
        }

        ret = close(sockfd_rx[i][1]);
        if (unlikely(ret == -1)) {
            log_error("close() error: %s", strerror(errno));
            return -1;
        }

        ret = close(sockfd_tx[i][0]);
        if (unlikely(ret == -1)) {
            log_error("close() error: %s", strerror(errno));
            return -1;
        }

        ret = close(sockfd_tx[i][1]);
        if (unlikely(ret == -1)) {
            log_error("close() error: %s", strerror(errno));
            return -1;
        }
    }

    ret = io_exit();
    if (unlikely(ret == -1))
    {
        log_error("io_exit() error");
        return -1;
    }

    return 0;
}

int main(int argc, char **argv)
{
    log_set_level_from_env();

    uint8_t nf_id;
    int ret;

    ret = rte_eal_init(argc, argv);
    if (unlikely(ret == -1))
    {
        log_error("rte_eal_init() error: %s", rte_strerror(rte_errno));
        goto error_0;
    }

    argc -= ret;
    argv += ret;

    if (unlikely(argc == 1))
    {
        log_error("Network Function ID not provided");
        goto error_1;
    }

    errno = 0;
    nf_id = strtol(argv[1], NULL, 10);
    if (unlikely(errno != 0 || nf_id < 1))
    {
        log_error("Invalid value for Network Function ID");
        goto error_1;
    }

    ret = nf(nf_id);
    if (unlikely(ret == -1))
    {
        log_error("nf() error");
        goto error_1;
    }

    ret = rte_eal_cleanup();
    if (unlikely(ret < 0))
    {
        log_error("rte_eal_cleanup() error: %s", rte_strerror(-ret));
        goto error_0;
    }

    return 0;

error_1:
    rte_eal_cleanup();
error_0:
    return 1;
}
