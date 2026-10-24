# Team 1 Prototype Code

This folder contains the Arduino code for our prototype. It's entirely separate from the surrounding ROS2 project.

## Setup

## Deploy Process

To deploy code from this repo to the Arduino Uno Q, make sure that this repository is cloned into `/home/arduino/SEC-2027-StockCarRace`. To deploy, switch to the `parallel-design-1` branch, run `git pull`, and then, run the `install.sh` script in the `prototype-code` folder to copy the app to the Arduino App Lab folder.

When using this project for the first time on an Arduino Uno Q, clone the repo and switch to the parallel-design-1 branch. Then, run `prototype-code/setup.sh` to symlink the app to the ArduinoApps directory so that it can be detected by Arduino App Lab.

(these setup instructions are speculative at the moment.)
