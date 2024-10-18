# Base image with necessary dependencies
FROM ubuntu:20.04

# Ubuntu 20.04 image hangs unless timezone is set
RUN DEBIAN_FRONTEND=noninteractive \
    ln -fs /usr/share/zoneinfo/America/New_York /etc/localtime

# Install dependencies
RUN apt-get update && \
    apt-get install -y \
    build-essential \
    clang \
    llvm \
    libelf-dev \
    libpcap-dev \
    iproute2 \
    libnuma-dev \
    dpdk-dev \
    libbpf-dev

# Set environment variables
ENV DPDK_HOME=/mydata/dpdk
ENV SPRIGHT_HOME=/mydata/spright
ENV LD_LIBRARY_PATH=/mydata/dpdk/build/lib:/mydata/dpdk/build/lib
ENV PATH="${PATH}:/mydata/spright/bin"

# Copy the SPRIGHT gateway component and dpdk/libbpf into the container
# Ensure these are in the correct directory structure under /mydata
COPY ./dpdk /mydata/dpdk
COPY ./libbpf /mydata/libbpf
COPY ./spright /mydata/spright

# Set the working directory to SPRIGHT directory
WORKDIR /mydata/spright/

# Expose any necessary ports (if needed)
EXPOSE 8000

# Set default command
CMD ["/bin/bash"]
