#!/bin/bash

CONTAINER_NAME="spright_container"

# Start the Docker container
docker run --name $CONTAINER_NAME \
  --privileged \  # Required for eBPF and NIC device access
  --net=host \    # Host network for low-latency networking
  --device=/dev/net/tun \  # NIC access for eBPF programs
  --device=/dev/uio0 \     # DPDK NIC device mapping
  -v /dev/hugepages:/dev/hugepages \  # Map host hugepage shared memory into container
  -v $MYMOUNT:/mydata \            # Map the entire /mydata directory to container's /mydata
  -it $1 \   # Container image name passed as argument
  /bin/bash  # Start the container interactively
