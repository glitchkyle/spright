#!/bin/bash

# Define container names and paths
CONTAINER_NAME=$2
IMAGE_NAME="spright_image"

if [ "$1" = "shm_mgr" ]; then
    COMMAND="/mydata/spright/bin/shm_mgr_rte_ring /mydata/spright/configs/shm_mgr_config.cfg"
elif [ "$1" = "gateway" ]; then
    COMMAND="/mydata/spright/bin/gateway_rte_ring"
elif [ "$1" = "nf" ]; then
    COMMAND="/mydata/spright/bin/nf_rte_ring $3"  # Pass NF_ID as $3
else
    echo "Invalid component: $1"
    exit 1
fi

# Start the Docker container with the appropriate command
sudo docker run \
  --detach=true \
  --privileged \
  --name $CONTAINER_NAME \
  --network bridge \
  --device=/dev/net/tun \
  --device=/dev/uio0 \
  --volume /dev/hugepages:/dev/hugepages \
  $IMAGE_NAME $COMMAND
