#! /bin/bash

docker run -it --user ros --name env-2027 -v $PWD/src:/home/ros/ws:rw -v /tmp/.X11-unix:/tmp/.X11-unix:rw --network=host --ipc=host --env=DISPLAY vt-cro/ros-env
