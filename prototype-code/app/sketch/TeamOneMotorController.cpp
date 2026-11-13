#include "TeamOneMotorController.hpp"
#include <Arduino.h>

namespace {
constexpr int NEUTRAL_PULSE_US = 1500;
constexpr float MAX_COMMAND = 1.0;
} // namespace

// The list after ':' initializes the const members from the constructor
// arguments.
TeamOneMotorController::TeamOneMotorController(uint8_t signalPin, bool reversed)
    : signalPin(signalPin), reversed(reversed) {}

void TeamOneMotorController::begin() {
  signal.attach(signalPin);
  stop();
}

void TeamOneMotorController::runOpenLoop(float velocity) {
  float command = constrain(velocity, -MAX_COMMAND, MAX_COMMAND);
  if (reversed) {
    command = -command;
  }

  // Map -1.0..0..1.0 to the ESC's minimum, neutral, and maximum pulse widths.
  // Scale each side from neutral separately so the endpoints can differ.
  const long span = command < 0 ? NEUTRAL_PULSE_US - MIN_PULSE_US
                                : MAX_PULSE_US - NEUTRAL_PULSE_US;
  writePulseMicroseconds(NEUTRAL_PULSE_US + command * span / MAX_COMMAND);
}

void TeamOneMotorController::stop() {
  // Servo's timer repeats this pulse until the width changes or we detach.
  writePulseMicroseconds(NEUTRAL_PULSE_US);
}

void TeamOneMotorController::writePulseMicroseconds(int pulseUs) {
  signal.writeMicroseconds(constrain(pulseUs, MIN_PULSE_US, MAX_PULSE_US));
}
