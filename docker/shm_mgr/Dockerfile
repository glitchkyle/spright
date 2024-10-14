# Base image with necessary dependencies
FROM ubuntu:20.04

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
ENV PATH="${PATH}:/mydata/spright/bin"

# Copy the SPRIGHT shared memory manager component, dpdk, and libbpf
COPY ./dpdk /mydata/dpdk
COPY ./libbpf /mydata/libbpf
COPY ./spright /mydata/spright

# Set the working directory to SPRIGHT shared memory manager and compile if needed
WORKDIR /mydata/spright/shm_mgr

# Set default command
CMD ["/bin/bash"]
