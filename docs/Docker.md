# Running SPRIGHT in Docker

# TODOs

1. Move the build and installation of SPRIGHT dependencies (e.g., `dpdk`, `libbpf`) inside the Docker image to avoid host dependency setup.
2. Automate `.cfg` file configuration to dynamically set `hostname` and `ip_address` for components that need it.
3. Automate value initialization of `addr.sin_addr.s_addr` in `io_sk_msg.c`
4. Ensure all SPRIGHT containers operate in different namespaces to enable inter-component communication.
