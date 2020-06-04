#!/bin/bash

set -e

echo "Running Bazel build"

bazel build ...

# Copy binaries into docker directory
echo "Copying my binaries here"
cp -fv bazel-bin/src/chunkserver/chunkserver ./docker/chunkserver
cp -fv bazel-bin/src/master/master ./docker/master

# Build docker images
echo "Building docker image for chunkserver"
docker build --tag chunkserver docker/chunkserver

echo "Building docker image for master"
docker build --tag master docker/master
