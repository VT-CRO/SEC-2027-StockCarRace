#ifndef TANK_DRIVETRAIN_H_
#define TANK_DRIVETRAIN_H_

#include "MotorController.hpp"

/*
A TankDrivetrain takes two MotorControllers and allows the robot to be
controlled more easily with a helper function.
*/
class TankDrivetrain {
public:
  /*
   * @brief Instantiates a new TankDrivetrain
   * @details Both MotorController parameters assume that a positive velocity
   * value will move the robot forward.
   * @param leftMotor The MotorController bound to the left motor
   * @param rightMotor The MotorController bound to the right motor
   */
  TankDrivetrain(MotorController &leftMotor, MotorController &rightMotor);

  /*
   * @brief Drive the motors at certain speeds
   * @param leftVelocity the velocity at which to run the left motor, from -255
   * (full speed backward) to 255 (full speed forward)
   * @param rightVelocity the velocity at which to run the right motor, from
   * -255 (full speed backward) to 255 (full speed forward)
   */
  void runOpenLoop(int leftVelocity, int rightVelocity);

  /*
   * @brief Stop both drive motors.
   */
  void stop();

private:
  MotorController &leftMotor;
  MotorController &rightMotor;
};

#endif