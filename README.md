# SEC-2027-StockCarRace's Parallel-Team-Template
A skeleton for the parallel teams to use when writing the software for their respective robots.

## Structure Overview
In general, the robot code should follow the following structure:

```
Autonomous (Behavior Tree / Nav2 / Localization)
        |
        v
/cmd_vel and other command topics
        |
        v
controller
        |
        v
ros2_control command interfaces
        |
        v
hardware (hardware interface)
        |
        v
motors / servos / embedded hardware
```

## Breakdown
The directory is composed of 4 basic packages:

**autonomous**: The package where the commands, behavior trees, and autonomous launch files will live.

**bringup**: The package where the entire robot's launch file will live.

**controller**: The package where the robot's controller will live.

**hardware**: The package where the robot code will leave.

See individual packages for further description.

**Note that the CMake files aren't completed. This is intentional**