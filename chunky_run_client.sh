#!/bin/bash
CLIENT_INDEX=${1:-1}

echo "Launching client #${CLIENT_INDEX}"

NAME="client_$CLIENT_INDEX"

docker run --rm --name $NAME --network gfs-net client --master_ip master --master_port 50051
