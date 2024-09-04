#!/bin/bash

# README: several points you may pay attention to
# (1) If you use apache benchmark, remember to add a "/" at the end of the url
# Otherwise, apache benchmark cannot identify the url
# (2) Depending on the kernel version, apache benchmark may not scale up to 
# multiple cores; you may need to run multiple apache benchmarks in parallel
# to generate sufficient load
# (3) You need to install the SPRIGHT/Palladium deps in advance; This script
# doesn't install the deps for you; You also need to configure the NGINX ingress
# or Palladium ingress in advance
# (4) Make sure you can launch the load test manually, this script will then be
# tremendously helpful

if [ $# -ne 3 ]; then
        echo "This script is to automate Palladium Ingress experiment."
        echo "Running this script on your local machine."
        echo "Make sure you have ssh access configured."
        echo "usage: $0 <url> <time (seconds)> <clients>"
    exit 1
fi

# locust config
url=$1
time=$2
clients=$3

# ssh config
WORKER_1=sqi009@c220g5-110510.wisc.cloudlab.us
LOAD_GEN=sqi009@c220g5-110528.wisc.cloudlab.us

# dir config
BASE_DIR=/users/sqi009/spright
LOAD_GEN_DIR=/users/sqi009

# function chain config
FC_CONFIG=$BASE_DIR/cfg/example_mutli_node.cfg

# set up WORKER_1
ssh -q $WORKER_1 "tmux kill-session -t spright" # kill tmux session
ssh -q $WORKER_1 "tmux new-session -d -s spright -n demo" # create tmux session
ssh -q $WORKER_1 "tmux set-option -t spright remain-on-exit on"

# Create tmux panes remotely
ssh -q $WORKER_1 'bash -s' << ENDSSH
    echo "Creating tmux panes on $WORKER_1..."
    for j in {1..7}
    do
        tmux split-window -v -t spright:demo
        tmux select-layout -t spright:demo tiled
    done
ENDSSH

ssh -q $WORKER_1 'bash -s' <<ENDSSH
    for j in {1..7}
    do
        tmux send-keys -t spright:demo.\$j "cd $BASE_DIR" Enter
        sleep 0.1
    done

    echo "Running gateway and functions on $WORKER_1..."
    tmux send-keys -t spright:demo.1 "cd $BASE_DIR && sudo ./run.sh shm_mgr $FC_CONFIG" Enter
    sleep 1
    tmux send-keys -t spright:demo.2 "cd $BASE_DIR && sudo ./run.sh gateway" Enter
    sleep 10
    tmux send-keys -t spright:demo.3 "cd $BASE_DIR && sudo ./run.sh nf 1" Enter
    sleep 1
    tmux send-keys -t spright:demo.4 "cd $BASE_DIR && sudo ./run.sh nf 2" Enter
    sleep 1
    tmux send-keys -t spright:demo.5 "cd $BASE_DIR && sudo ./run.sh nf 3" Enter
    sleep 1
    tmux send-keys -t spright:demo.6 "cd $BASE_DIR && sudo ./run.sh nf 4" Enter
ENDSSH

sleep 10

# set up LOAD_GEN
ssh -q $LOAD_GEN "tmux kill-session -t spright" # kill tmux session
ssh -q $LOAD_GEN "tmux new-session -d -s spright -n demo" # create tmux session
ssh -q $LOAD_GEN "tmux set-option -t spright remain-on-exit on"

ssh -q $LOAD_GEN 'bash -s' << ENDSSH
    echo "Running $clients clients on apache benchmark"
    tmux send-keys -t spright:demo "ab -c $clients -t $time $url > ab_results.txt" Enter
ENDSSH

echo "sleep for $((time+10)) before downloading results"
sleep $((time+10))

# Download results
DOWNLOAD_DIR=result_$(date +%Y-%m-%d-%H-%M)
mkdir ./$DOWNLOAD_DIR
scp -q "$LOAD_GEN:$LOAD_GEN_DIR/ab*" ./$DOWNLOAD_DIR