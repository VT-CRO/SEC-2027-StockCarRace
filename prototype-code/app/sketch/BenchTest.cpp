#include "BenchTest.hpp"
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

void testEncoder(RelativeEncoder &encoder) {
  Monitor.print("Encoder position: ");
  Monitor.print(encoder.angleDegrees());
  Monitor.println(" degrees");
  delay(1000);
}