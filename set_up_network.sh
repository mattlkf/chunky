#!/bin/bash

# Create a docker network for our system
docker network create --driver bridge gfs-net

# Launch in detached mode the images
docker run -d --name greeter1 --network gfs-net chunkserver --other_ip greeter2 --other_port 50051
docker run -d --name greeter2 --network gfs-net chunkserver --other_ip greeter1 --other_port 50051
# To view, run "docker attach greeter2" in a separate terminal

# Start some greeters in the background, pinging each other
N_GREETERS=${1:-2}

# echo "Starting $N_GREETERS greeters"
# for i in $(seq 1 $N_GREETERS); do
#     GREETER_NAME="greeter-$i"
#     echo "Starting $GREETER_NAME"
#     docker run --name $GREETER_NAME --network gfs-net greeter --master_ip # TODO fill in
# done

# docker network rm gfs-net
