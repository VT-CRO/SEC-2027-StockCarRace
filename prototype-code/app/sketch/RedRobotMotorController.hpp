#ifndef RED_ROBOT_MOTOR_CONTROLLER_H_
#define RED_ROBOT_MOTOR_CONTROLLER_H_

#include "MotorController.hpp"
#include <stdint.h>

/*
 Motor controller implementation for the motors on the red prototype robot.

 I'm unsure what motor controller it uses so it is defined as "the ones on the
 red prototype bot."
 */
class RedRobotMotorController : public MotorController {
public:
  /*
   * @brief Instantiate a new RedRobotMotorController
   * @param forwardHighPin the pin that's set high when rotating forward, low
   * when rotating backward
   * @param backwardHighPin the pin that's set high when rotating backward, low
   * when rotating forward
   * @param speedPwmPin the pin where the desired speed is output using PWM.
   */
  RedRobotMotorController(uint8_t forwardHighPin, uint8_t backwardHighPin,
                          uint8_t speedPwmPin);

  void runOpenLoop(int velocity) override;

  void stop() override;

private:
  // The pin that's set high when rotating forward, low
  uint8_t forwardHighPin;
  // The pin that's set high when rotating backward, low
  uint8_t backwardHighPin;
  // The pin where the desired speed is output using PWM.
  uint8_t speedPwmPin;
};

#endif