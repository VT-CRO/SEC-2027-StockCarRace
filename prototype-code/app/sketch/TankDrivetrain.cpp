#include "TankDrivetrain.hpp"

TankDrivetrain::TankDrivetrain(MotorController &leftMotor,
                               MotorController &rightMotor)
    : leftMotor(leftMotor), rightMotor(rightMotor) {};

void TankDrivetrain::runOpenLoop(int leftVelocity, int rightVelocity) {
  leftMotor.runOpenLoop(leftVelocity);
  rightMotor.runOpenLoop(rightVelocity);
}

void TankDrivetrain::stop() {
  leftMotor.stop();
  rightMotor.stop();
}