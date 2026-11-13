#ifndef TEAM_ONE_MOTOR_CONTROLLER_H_
#define TEAM_ONE_MOTOR_CONTROLLER_H_

#include "MotorController.hpp"
#include <Servo.h>
#include <stdint.h>

// Controls one Repeat AM32 drive ESC using bidirectional RC servo pulses.
// The Servo library repeats a pulse on the signal pin about every 20 ms.
// The ESC reads how long the pin stays high: shorter
// pulses command one direction, and longer pulses command the other.
// With reversal disabled, runOpenLoop() maps -1.0..0..1.0 to
// the low endpoint, configured neutral, and high endpoint. Actual speed depends on
// load.
class TeamOneMotorController : public MotorController {
public:
  static constexpr int MIN_PULSE_US = 1000; // i dont think these are right
  static constexpr int MAX_PULSE_US = 2000;

  explicit TeamOneMotorController(uint8_t signalPin, bool reversed = false,
                                  int neutralPulseUs = 1500);

  // Call from setup() and allow the ESC to arm at neutral before driving.
  void begin();

  void runOpenLoop(float velocity) override;

  void stop() override;

  // Raw ESC pulse for bench calibration, independent of wheel reversal.
  void writePulseMicroseconds(int pulseUs);

private:
  const uint8_t signalPin;
  const bool reversed;
  const int neutralPulseUs;
  Servo signal;
};

#endif
