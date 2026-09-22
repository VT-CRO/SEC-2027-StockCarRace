#! /bin/bash

# Create the workspace as the invoking user. If this is left to `docker run`,
# the daemon creates it as root and the container's ros user cannot write to it.
mkdir -p "$PWD/src"

docker run -it --user ros --name env-2027 -v "$PWD/src:/home/ros/ws:rw" -v /tmp/.X11-unix:/tmp/.X11-unix:rw --network=host --ipc=host --env=DISPLAY --gpus all vt-cro/ros-env
