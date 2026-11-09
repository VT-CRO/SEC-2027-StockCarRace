#include "TeamOneMotorController.hpp"
#include <Arduino.h>

namespace {
constexpr int MIN_PULSE_US = 1000;
constexpr int NEUTRAL_PULSE_US = 1500;
constexpr int MAX_PULSE_US = 2000;
constexpr int MAX_COMMAND = 255;
}

// The list after ':' initializes the const members from the constructor arguments.
TeamOneMotorController::TeamOneMotorController(uint8_t signalPin, bool reversed)
    : signalPin(signalPin), reversed(reversed) {}

void TeamOneMotorController::begin() {
  signal.attach(signalPin);
  stop();
}

void TeamOneMotorController::runOpenLoop(int velocity) {
  int command = constrain(velocity, -MAX_COMMAND, MAX_COMMAND);
  if (reversed) {
    command = -command;
  }

  // Map -255..0..255 to the ESC's minimum, neutral, and maximum pulse widths.
  // Scale each side from neutral separately so the endpoints can differ.
  const long span = command < 0 ? NEUTRAL_PULSE_US - MIN_PULSE_US
                                : MAX_PULSE_US - NEUTRAL_PULSE_US;
  signal.writeMicroseconds(NEUTRAL_PULSE_US + command * span / MAX_COMMAND);
}

void TeamOneMotorController::stop() {
  runOpenLoop(0);
}
