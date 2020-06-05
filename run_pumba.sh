#!/bin/bash

docker run -it --rm  -v /var/run/docker.sock:/var/run/docker.sock gaiaadm/pumba --interval=10s --random --log-level=info kill --signal=SIGKILL "re2:^chunkserver"
