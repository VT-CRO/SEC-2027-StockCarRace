#include "TeamOneMotorController.hpp"
#include <Arduino.h>

namespace {
constexpr float MAX_COMMAND = 1.0;
} // namespace

// The list after ':' initializes the const members from the constructor
// arguments.
TeamOneMotorController::TeamOneMotorController(uint8_t signalPin, bool reversed,
                                               int neutralPulseUs)
    : signalPin(signalPin), reversed(reversed),
      neutralPulseUs(constrain(neutralPulseUs, MIN_PULSE_US, MAX_PULSE_US)) {}

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
  const long span = command < 0 ? neutralPulseUs - MIN_PULSE_US
                                : MAX_PULSE_US - neutralPulseUs;
  writePulseMicroseconds(neutralPulseUs + command * span / MAX_COMMAND);
}

void TeamOneMotorController::stop() {
  // Servo's timer repeats this pulse until the width changes or we detach.
  writePulseMicroseconds(neutralPulseUs);
}

void TeamOneMotorController::writePulseMicroseconds(int pulseUs) {
  signal.writeMicroseconds(constrain(pulseUs, MIN_PULSE_US, MAX_PULSE_US));
}
