#include "RedRobotMotorController.hpp"
#include "Arduino.h"

#include <stdint.h>

RedRobotMotorController::RedRobotMotorController(uint8_t forwardHighPin,
                                                 uint8_t backwardHighPin,
                                                 uint8_t speedPwmPin)
    : forwardHighPin(forwardHighPin), backwardHighPin(backwardHighPin),
      speedPwmPin(speedPwmPin) {
  pinMode(forwardHighPin, OUTPUT);
  pinMode(backwardHighPin, OUTPUT);
  pinMode(speedPwmPin, OUTPUT);
}

void RedRobotMotorController::runOpenLoop(int velocity) {
  // Assume forward
  uint8_t highPin = forwardHighPin;
  uint8_t lowPin = backwardHighPin;
  // If we're actually going backward, switch the pins
  if (velocity < 0) {
    highPin = backwardHighPin;
    lowPin = forwardHighPin;
  }

  unsigned int speedPwmValue = abs(velocity);
  if (speedPwmValue > 255) {
    speedPwmValue = 255;
  }

  digitalWrite(highPin, HIGH);
  digitalWrite(lowPin, LOW);
  analogWrite(speedPwmPin, speedPwmValue);
}

void RedRobotMotorController::stop() {
  digitalWrite(forwardHighPin, LOW);
  digitalWrite(backwardHighPin, LOW);
  analogWrite(speedPwmPin, 0);
}