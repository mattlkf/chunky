#!/bin/bash

echo "Launching client"

docker run --name client --network gfs-net client --master_ip master --master_port 50051
