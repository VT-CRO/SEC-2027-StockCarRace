# Bringup
A skeleton for the bringup behavior of the robot

## Breakdown
This package will contain the **launch files** of the entire robot.

**What is a launch file?**: A launch file is a file that the user runs to launch a group of packages / nodes at once, like a "start" button. For example, if I wanted to start the entire robot, I would write a launch file that would launch the autonomous behavior, the controller, the hardware, etc, of the robot.

These files are executed by the user and are often written in Python. Different libraries have their own launch file that you can use.