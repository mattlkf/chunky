#!/bin/bash

set -e

echo "Running Bazel build"

bazel build ...

echo "Copying my binaries here"
cp -fv bazel-bin/src/client/greeter_client ./docker/chunkserver

echo "Building docker image"
docker build --tag chunkserver docker/chunkserver
