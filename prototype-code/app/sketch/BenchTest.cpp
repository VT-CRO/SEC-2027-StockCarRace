#include "BenchTest.hpp"
#include "MonitorInput.hpp"
#include <Arduino.h>
#include <Arduino_RouterBridge.h>
#include <cmath>
#include <cstdlib>
#include <cstring>

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

void runHeadingBenchTest(TankDrivetrain &drivetrain, IMU &imu, bool yawReversed) {
  constexpr unsigned long SAMPLE_MS = 10;
  constexpr float MAX_TRIM = 0.06f;
  constexpr float DEG_PER_RAD = 57.29578f;
  constexpr float MAX_DEVIATION_DEG = 20.0f;
  const float yawSign = yawReversed ? -1.0f : 1.0f;
  MonitorCommandReader reader(Monitor);
  discardMonitorInput(Monitor);
  Monitor.println("Align the car and keep it still. Send run 8 0.25 0.2 or yaw.");
  Monitor.println("Format: run <seconds> <throttle> <Kp>. yaw checks direction with motors at neutral.");

  while (true) {
    const auto command = reader.poll();
    if (command == MonitorCommandReader::Result::Pending) {
      delay(10);
      continue;
    }
    const char *text = reader.command();
    const bool yawCheck = command == MonitorCommandReader::Result::Command &&
                          strcmp(text, "yaw") == 0;
    float seconds = 0.0f, throttle = 0.0f, kp = 0.0f;
    if (!yawCheck) {
      if (command == MonitorCommandReader::Result::Command &&
          strncmp(text, "run ", 4) == 0) {
        const char *field = text + 4;
        float *const values[] = {&seconds, &throttle, &kp};
        for (float *value : values) {
          char *end = nullptr;
          *value = strtof(field, &end);
          if (end == field || (*end != ' ' && *end != '\0')) {
            seconds = 0.0f;
            break;
          }
          field = end;
        }
        while (*field == ' ') {
          ++field;
        }
        if (*field != '\0') {
          seconds = 0.0f;
        }
      }
      if (!(seconds >= 1.0f && seconds <= 15.0f && throttle >= 0.10f &&
            throttle <= 0.35f && kp >= 0.0f && kp <= 1.0f)) {
        Monitor.println("Use yaw or run <1-15 seconds> <0.10-0.35 throttle> <0-1 Kp>.");
        continue;
      }
    }

    Monitor.println("Keep the car still while the gyro calibrates.");
    if (!imu.calibrate()) {
      continue;
    }

    imu.resetHeading(0.0f);
    if (yawCheck) {
      Monitor.println("Turn the car left by hand now. Corrected yaw should become positive.");
    }
    const unsigned long runMs = yawCheck ? 5000 : static_cast<unsigned long>(seconds * 1000.0f);
    const char *result = "Heading trial complete.";
    float headingDeg = 0.0f;
    float peakDeviationDeg = 0.0f;
    const unsigned long started = millis();
    while (millis() - started < runMs) {
      if (!imu.update()) {
        result = "Heading trial stopped: IMU read failed.";
        break;
      }
      const float yaw = imu.getHeadingRads() * yawSign;
      headingDeg = yaw * DEG_PER_RAD;
      peakDeviationDeg = fmaxf(peakDeviationDeg, fabsf(headingDeg));
      if (!yawCheck && peakDeviationDeg >= MAX_DEVIATION_DEG) {
        result = "Heading trial stopped: yaw exceeded 20 degrees.";
        break;
      }
      // Positive yaw is a left turn. Add left throttle to steer back right.
      if (!yawCheck) {
        const float trim = constrain(kp * yaw, -MAX_TRIM, MAX_TRIM);
        drivetrain.runOpenLoop(throttle + trim, throttle - trim);
      }
      delay(SAMPLE_MS);
    }
    drivetrain.stop();

    Monitor.println(result);
    Monitor.print("Final yaw (deg): ");
    Monitor.println(headingDeg);
    Monitor.print("Peak yaw deviation (deg): ");
    Monitor.println(peakDeviationDeg);
    discardMonitorInput(Monitor);
  }
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
