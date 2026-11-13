#include "BenchTest.hpp"
#include "MonitorInput.hpp"
#include <Arduino.h>
#include <Arduino_RouterBridge.h>

namespace {
constexpr float TEST_COMMAND = 0.25;
constexpr unsigned long TEST_RUN_MS = 500;
constexpr unsigned long NEUTRAL_PAUSE_MS = 1000;

void testWheel(TankDrivetrain &drivetrain, const char *label, float leftCommand,
               float rightCommand) {
  Monitor.println(label);
  drivetrain.runOpenLoop(leftCommand, rightCommand);
  delay(TEST_RUN_MS);
  drivetrain.stop();
  delay(NEUTRAL_PAUSE_MS);
}
} // namespace

void runBenchTest(TankDrivetrain &drivetrain) {
  testWheel(drivetrain, "Left forward", TEST_COMMAND, 0);
  testWheel(drivetrain, "Left reverse", -TEST_COMMAND, 0);
  testWheel(drivetrain, "Right forward", 0, TEST_COMMAND);
  testWheel(drivetrain, "Right reverse", 0, -TEST_COMMAND);
  Monitor.println("Bench test complete; holding neutral.");
}

namespace {
// Allow slow rotation while still stopping if the count never reaches target.
constexpr unsigned long ROTATION_TIMEOUT_MS = 15000;
constexpr unsigned long MARK_INSPECTION_MS = 2000;

void printEncoders(const RelativeEncoder &left, const RelativeEncoder &right) {
  Monitor.print("Left: ");
  Monitor.print(left.count());
  Monitor.print(" counts, ");
  Monitor.print(left.angleDegrees());
  Monitor.print(" deg | Right: ");
  Monitor.print(right.count());
  Monitor.print(" counts, ");
  Monitor.print(right.angleDegrees());
  Monitor.println(" deg");
}

bool moveToCount(TankDrivetrain &drivetrain, const RelativeEncoder &encoder,
                 int32_t target, float leftCommand, float rightCommand) {
  const bool forward = leftCommand > 0 || rightCommand > 0;
  const unsigned long started = millis();
  drivetrain.runOpenLoop(leftCommand, rightCommand);
  bool reached = false;
  while (millis() - started < ROTATION_TIMEOUT_MS) {
    const int32_t count = encoder.count();
    if (forward ? count >= target : count <= target) {
      reached = true;
      break;
    }
    delay(1);
  }
  drivetrain.stop();
  delay(NEUTRAL_PAUSE_MS);
  return reached;
}

bool testEncoderRotation(TankDrivetrain &drivetrain, RelativeEncoder &left,
                         RelativeEncoder &right, const char *label,
                         bool testLeft, float forward, float reverse) {
  waitForMonitorMessage(Monitor, label);
  left.reset();
  right.reset();
  const RelativeEncoder &encoder = testLeft ? left : right;
  const int32_t target = encoder.countsPerRevolution();
  const bool forwardReached = moveToCount(drivetrain, encoder, target,
                                          testLeft ? forward : 0.0f,
                                          testLeft ? 0.0f : forward);
  Monitor.println(forwardReached ? "Forward target crossed. Check the tape mark."
                                 : "Forward target timed out. Test stopped.");
  printEncoders(left, right);
  if (!forwardReached) {
    return false;
  }

  delay(MARK_INSPECTION_MS);
  const bool returned = moveToCount(drivetrain, encoder, 0,
                                    testLeft ? reverse : 0.0f,
                                    testLeft ? 0.0f : reverse);
  Monitor.println(returned ? "Starting count crossed. Check the final tape mark."
                           : "Return target timed out. Test stopped.");
  printEncoders(left, right);
  return returned;
}
} // namespace

void runEncoderBenchTest(TankDrivetrain &drivetrain, RelativeEncoder &left,
                         RelativeEncoder &right) {
  // With the measured neutral and reversal settings these command roughly
  // 1400 us on the low side and 1550 us on the high side.
  constexpr float LOW_SIDE = 0.11f;
  constexpr float HIGH_SIDE = 0.182f;

  drivetrain.stop();
  if (testEncoderRotation(drivetrain, left, right,
                          "Raise both wheels and let the ESCs arm. Send a message to test the left wheel.",
                          true, LOW_SIDE, -HIGH_SIDE) &&
      testEncoderRotation(drivetrain, left, right,
                          "Send a message to test the right wheel.",
                          false, HIGH_SIDE, -LOW_SIDE)) {
    Monitor.println("Encoder test complete. Both ESCs are holding neutral.");
  }
}
