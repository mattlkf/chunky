#!/bin/bash

echo Killing all containers..
docker kill $(docker ps -q)

echo Removing all containers..
docker rm $(docker ps -aq)

echo Removing the gfs-net network
docker network rm gfs-net

echo Removing all unused volumes
docker volume prune
