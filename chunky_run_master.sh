#!/bin/bash

echo "Creating network"
docker network create --driver bridge gfs-net

echo "Launching master"
docker run --name master --network gfs-net master --self_ip master --self_port 50051
