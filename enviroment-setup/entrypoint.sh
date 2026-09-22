#! /bin/bash

set -e

source /opt/ros/humble/setup.bash

ulimit -n 1024

echo "Provided arguments: $@"

exec "$@"
