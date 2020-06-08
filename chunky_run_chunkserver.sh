#!/bin/bash
CHUNKSERVER_INDEX=${1:-1}

echo "Launching chunkserver #${CHUNKSERVER_INDEX}"

NAME="chunkserver_$CHUNKSERVER_INDEX"
VOLUME="vol_$CHUNKSERVER_INDEX"
docker run --rm --mount source=${VOLUME},target=/chunk_storage --name $NAME --network gfs-net chunkserver --self_ip $NAME --master_ip master --master_port 50051
