# Use Ubuntu 20.04 as the base image, as specified in the YAML
FROM ubuntu:20.04

# Ubuntu 20.04 image hangs unless timezone is set
RUN DEBIAN_FRONTEND=noninteractive \
    ln -fs /usr/share/zoneinfo/America/New_York /etc/localtime

# Install any necessary dependencies
RUN apt-get update && \
    apt-get install -y \
    libelf-dev \
    libpcap-dev \
    libnuma-dev \
    libconfig-dev \
    dpdk-dev \
    pkg-config \
    iproute2

# Set up working directory for SPRIGHT
WORKDIR /mydata/spright

# Keep the container running
CMD ["bash", "-c", "while true; do sleep 30; done;"]
