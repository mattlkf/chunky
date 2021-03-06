#!/bin/bash

# How many chunkservers to start
N_CHUNKSERVERS=${1:-2}

# Create a docker network for our system
echo "Creating network"
docker network create --driver bridge gfs-net

echo "Launching master"
docker run -d --name master --network gfs-net master --self_ip master --self_port 50051

# Launch in detached mode the chunkservers
echo "Launching chunkservers"
for i in $(seq 1 $N_CHUNKSERVERS); do
    cs_name="chunkserver$i"
    echo "Name: $cs_name"
    volume_name="vol_$i"
    docker run -d --mount source=${volume_name},target=/chunk_storage --name $cs_name --network gfs-net chunkserver --self_ip $cs_name --master_ip master --master_port 50051
done

# docker container ls -a
docker attach chunkserver2


# To view any instance, run "docker attach <name>" in a separate terminal

# Start some greeters in the background, pinging each other

# echo "Starting $N_GREETERS greeters"
# for i in $(seq 1 $N_GREETERS); do
#     GREETER_NAME="greeter-$i"
#     echo "Starting $GREETER_NAME"
#     docker run --name $GREETER_NAME --network gfs-net greeter --master_ip # TODO fill in
# done

# docker network rm gfs-net
