#!/bin/bash

# How many chunkservers to start
N_CHUNKSERVERS=${1:-2}

# Launch in detached mode the chunkservers
echo "Launching chunkservers"
for i in $(seq 1 $N_CHUNKSERVERS); do
    cs_name="chunkserver_$i"
    echo "Name: $cs_name"
    volume_name="vol_$i"
    docker run -d --rm --mount source=${volume_name},target=/chunk_storage --name $cs_name --network gfs-net chunkserver --self_ip $cs_name --master_ip master --master_port 50051
done
