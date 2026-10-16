# Controller
A skeleton for the controller of the robot

## Breakdown
This package contains the controller of the robot, using the `ros2_controls` package.

**What is a controller?**: A controller takes data from the autonomous (localization data, commands, etc), and converts it into a form that the hardware can use. For example, if the autonomous behavior is publishing to the topic `/cmd_vel`, the controller takes that value and converts it into commands for the hardware to execute.