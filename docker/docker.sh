#!/bin/bash

IMAGE_NAME=$1
CONTAINER_NAME=$2

sudo docker run -d \
  --name $CONTAINER_NAME \
  --privileged \
  --network bridge \
  --device=/dev/net/tun \
  --device=/dev/uio0 \
  -v /dev/hugepages:/dev/hugepages \
  -v /var/run/dpdk:/var/run/dpdk \
  -v /lib/x86_64-linux-gnu:/lib/x86_64-linux-gnu \
  -v /usr/local/lib/x86_64-linux-gnu:/usr/local/lib/x86_64-linux-gnu \
  -v /usr/lib/x86_64-linux-gnu:/usr/lib/x86_64-linux-gnu \
  -v /usr/lib64:/usr/lib64 \
  -v /mydata/spright:/mydata/spright \
  $IMAGE_NAME
