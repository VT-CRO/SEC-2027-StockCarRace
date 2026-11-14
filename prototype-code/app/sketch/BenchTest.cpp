#include "BenchTest.hpp"
#include "MonitorInput.hpp"
#include "StraightDrive.hpp"
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

namespace {
struct RpmTrial {
  float seconds;
  float rpm;
  float leftBase;
  float rightBase;
  float kp;
  float ki;
  float yawKp;
};

bool readTrialFields(const char *fields, float *const *values, size_t count) {
  for (size_t i = 0; i < count; ++i) {
    char *end = nullptr;
    *values[i] = strtof(fields, &end);
    if (end == fields || !std::isfinite(*values[i]) ||
        (*end != ' ' && *end != '\0')) {
      return false;
    }
    fields = end;
  }
  while (*fields == ' ') {
    ++fields;
  }
  return *fields == '\0';
}

bool readRpmTrial(const char *fields, RpmTrial &trial, bool combined) {
  float *const values[] = {&trial.seconds, &trial.rpm, &trial.leftBase,
                           &trial.rightBase, &trial.kp, &trial.ki, &trial.yawKp};
  return readTrialFields(fields, values, combined ? 7 : 6) &&
         trial.seconds >= 1.0f && trial.seconds <= 15.0f &&
         trial.rpm > 0.0f && trial.rpm <= 300.0f &&
         trial.leftBase >= 0.10f && trial.leftBase <= 0.35f &&
         trial.rightBase >= 0.10f && trial.rightBase <= 0.35f &&
         trial.kp >= 0.0f && trial.kp <= 0.01f &&
         trial.ki >= 0.0f && trial.ki <= 0.01f &&
         trial.yawKp >= 0.0f && trial.yawKp <= 200.0f;
}

float speedThrottle(float base, float error, float dt, const RpmTrial &trial,
                    float &integral) {
  constexpr float MAX_THROTTLE = 0.45f;
  const float proportional = trial.kp * error;
  const float output = base + proportional + integral;
  const float integralStep = trial.ki * error * dt;
  // Limit integration to the available throttle headroom. Allow an error of
  // the opposite sign to unwind the integral when the output is saturated.
  integral += constrain(integralStep, fminf(0.0f, -output),
                        fmaxf(0.0f, MAX_THROTTLE - output));
  return constrain(base + proportional + integral, 0.0f, MAX_THROTTLE);
}

void runRpmTrial(TankDrivetrain &drivetrain, RelativeEncoder &left,
                 RelativeEncoder &right, IMU *imu, float yawSign,
                 const RpmTrial &trial) {
  constexpr unsigned long SPEED_SAMPLE_MS = 50;
  constexpr float DEG_PER_RAD = 57.29578f;
  if (imu) {
    Monitor.println("Keep the car still while the gyro calibrates.");
    if (!imu->calibrate()) {
      return;
    }
    imu->resetHeading(0.0f);
    if (!imu->update()) {
      Monitor.println("RPM trial stopped: IMU read failed before launch.");
      return;
    }
  }

  left.reset();
  right.reset();
  int32_t lastLeftCount = 0;
  int32_t lastRightCount = 0;
  float leftIntegral = 0.0f;
  float rightIntegral = 0.0f;
  float peakSpeedGap = 0.0f;
  float peakYawDeg = 0.0f;
  float yaw = 0.0f;
  const char *result = "RPM trial complete.";
  const unsigned long durationMs = static_cast<unsigned long>(trial.seconds * 1000.0f);
  const unsigned long started = millis();
  unsigned long lastSample = started;
  drivetrain.runOpenLoop(trial.leftBase, trial.rightBase);
  while (millis() - started < durationMs) {
    if (imu) {
      if (!imu->update()) {
        result = "RPM trial stopped: IMU read failed.";
        break;
      }
      yaw = imu->getHeadingRads() * yawSign;
      peakYawDeg = fmaxf(peakYawDeg, fabsf(yaw * DEG_PER_RAD));
      if (peakYawDeg >= 20.0f) {
        result = "RPM trial stopped: yaw reached 20 degrees.";
        break;
      }
    }

    const unsigned long now = millis();
    const unsigned long intervalMs = now - lastSample;
    if (intervalMs >= SPEED_SAMPLE_MS && now - started < durationMs) {
      const int32_t leftCount = left.count();
      const int32_t rightCount = right.count();
      const float leftRpm = left.averageRpm(lastLeftCount, leftCount, intervalMs);
      const float rightRpm = right.averageRpm(lastRightCount, rightCount, intervalMs);
      peakSpeedGap = fmaxf(peakSpeedGap, fabsf(leftRpm - rightRpm));
      // Positive yaw is a left turn. Raise the left target to steer right.
      const float maxRpmTrim = trial.rpm * 0.25f;
      const float rpmTrim = constrain(trial.yawKp * yaw, -maxRpmTrim, maxRpmTrim);
      const float dt = intervalMs / 1000.0f;
      const float leftError = trial.rpm + rpmTrim - leftRpm;
      const float rightError = trial.rpm - rpmTrim - rightRpm;
      drivetrain.runOpenLoop(speedThrottle(trial.leftBase, leftError, dt, trial, leftIntegral),
                             speedThrottle(trial.rightBase, rightError, dt, trial, rightIntegral));
      lastLeftCount = leftCount;
      lastRightCount = rightCount;
      lastSample = now;
    }
    delay(10);
  }
  drivetrain.stop();
  const unsigned long elapsedMs = millis() - started;
  const int32_t leftEnd = left.count();
  const int32_t rightEnd = right.count();

  Monitor.println(result);
  Monitor.print("Left average RPM: ");
  Monitor.println(left.averageRpm(0, leftEnd, elapsedMs));
  Monitor.print("Right average RPM: ");
  Monitor.println(right.averageRpm(0, rightEnd, elapsedMs));
  Monitor.print("Peak wheel RPM difference: ");
  Monitor.println(peakSpeedGap);
  if (imu) {
    Monitor.print("Final yaw (deg): ");
    Monitor.println(yaw * DEG_PER_RAD);
    Monitor.print("Peak yaw deviation (deg): ");
    Monitor.println(peakYawDeg);
  }
}

struct StraightTrial {
  float seconds;
  float leftBase;
  float rightBase;
  float wheelKp;
  float yawKp;
};

bool readStraightTrial(const char *fields, StraightTrial &trial, bool combined) {
  float *const values[] = {&trial.seconds, &trial.leftBase, &trial.rightBase,
                           &trial.wheelKp, &trial.yawKp};
  return readTrialFields(fields, values, combined ? 5 : 4) &&
         trial.seconds >= 1.0f && trial.seconds <= 15.0f &&
         trial.leftBase >= 0.10f && trial.leftBase <= 0.35f &&
         trial.rightBase >= 0.10f && trial.rightBase <= 0.35f &&
         trial.wheelKp >= 0.0f && trial.wheelKp <= 1.0f &&
         trial.yawKp >= 0.0f && trial.yawKp <= 2.0f;
}

void runStraightTrial(TankDrivetrain &drivetrain, RelativeEncoder &left,
                      RelativeEncoder &right, IMU *imu, float yawSign,
                      const StraightTrial &trial) {
  const StraightDriveTuning tuning{trial.leftBase, trial.rightBase,
                                   trial.wheelKp, trial.yawKp};
  constexpr float DEG_PER_RAD = 57.29578f;
  constexpr unsigned long SAMPLE_MS = 10;
  const unsigned long durationMs = static_cast<unsigned long>(trial.seconds * 1000.0f);

  if (imu) {
    Monitor.println("Keep the car still while the gyro calibrates.");
    if (!imu->calibrate()) {
      return;
    }
    imu->resetHeading(0.0f);
    if (!imu->update()) {
      Monitor.println("Straight test stopped: IMU read failed before launch.");
      return;
    }
  }

  left.reset();
  right.reset();
  const char *result = "Straight trial complete.";
  float yawDeg = 0.0f;
  float peakYawDeg = 0.0f;
  float peakWheelGap = 0.0f;
  const unsigned long started = millis();
  while (millis() - started < durationMs) {
    float yaw = 0.0f;
    if (imu) {
      if (!imu->update()) {
        result = "Straight trial stopped: IMU read failed.";
        break;
      }
      yaw = imu->getHeadingRads() * yawSign;
      yawDeg = yaw * DEG_PER_RAD;
      peakYawDeg = fmaxf(peakYawDeg, fabsf(yawDeg));
      if (peakYawDeg >= 20.0f) {
        result = "Straight trial stopped: yaw exceeded 20 degrees.";
        break;
      }
    }

    const float wheelGap = left.revolutions() - right.revolutions();
    peakWheelGap = fmaxf(peakWheelGap, fabsf(wheelGap));
    driveStraight(drivetrain, tuning, wheelGap, yaw);
    delay(SAMPLE_MS);
  }
  drivetrain.stop();

  Monitor.println(result);
  printEncoders(left, right);
  Monitor.print("Peak wheel travel difference (rev): ");
  Monitor.println(peakWheelGap, 3);
  if (imu) {
    Monitor.print("Final yaw (deg): ");
    Monitor.println(yawDeg);
    Monitor.print("Peak yaw deviation (deg): ");
    Monitor.println(peakYawDeg);
  }
}
} // namespace

void runStraightBenchTest(TankDrivetrain &drivetrain, RelativeEncoder &left,
                          RelativeEncoder &right, IMU *imu, bool yawReversed) {
  drivetrain.stop();
  MonitorCommandReader reader(Monitor);
  discardMonitorInput(Monitor);
  Monitor.println("Align the car on a clear straight and let the ESCs arm.");
  Monitor.println("wheels <seconds> <left> <right> <wheel Kp>");
  Monitor.println("both <seconds> <left> <right> <wheel Kp> <yaw Kp>");
  Monitor.println("rpm <seconds> <RPM> <left base> <right base> <Kp> <Ki>");
  Monitor.println("rpmimu <seconds> <RPM> <left base> <right base> <Kp> <Ki> <yaw Kp>");
  if (!imu) {
    Monitor.println("IMU unavailable. Use wheels or rpm for encoder-only trials.");
  }

  while (true) {
    const auto status = reader.poll();
    if (status == MonitorCommandReader::Result::Pending) {
      delay(10);
      continue;
    }

    const char *command = reader.command();
    const bool rpmOnly = strncmp(command, "rpm ", 4) == 0;
    const bool rpmWithImu = strncmp(command, "rpmimu ", 7) == 0;
    if (status == MonitorCommandReader::Result::Command && (rpmOnly || rpmWithImu)) {
      RpmTrial trial{};
      if (!readRpmTrial(command + (rpmWithImu ? 7 : 4), trial, rpmWithImu)) {
        Monitor.println("Use 1-15 seconds, 0-300 RPM (above zero), base 0.10-0.35, Kp/Ki 0-0.01, yaw Kp 0-200.");
        continue;
      }
      if (rpmWithImu && (!imu || (trial.kp == 0.0f && trial.ki == 0.0f))) {
        Monitor.println("rpmimu needs a working IMU and a positive speed gain.");
        continue;
      }
      runRpmTrial(drivetrain, left, right, rpmWithImu ? imu : nullptr,
                  yawReversed ? -1.0f : 1.0f, trial);
      discardMonitorInput(Monitor);
      continue;
    }
    const bool wheels = strncmp(command, "wheels ", 7) == 0;
    const bool combined = strncmp(command, "both ", 5) == 0;
    StraightTrial trial{};
    if (status != MonitorCommandReader::Result::Command || (!wheels && !combined) ||
        !readStraightTrial(command + (combined ? 5 : 7), trial, combined)) {
      Monitor.println("Use 1-15 seconds, base throttles 0.10-0.35, wheel Kp 0-1, yaw Kp 0-2.");
      continue;
    }
    if (combined && (!imu || trial.wheelKp == 0.0f)) {
      Monitor.println("Combined trials need a working IMU and a positive wheel Kp.");
      continue;
    }

    runStraightTrial(drivetrain, left, right, combined ? imu : nullptr,
                     yawReversed ? -1.0f : 1.0f, trial);
    discardMonitorInput(Monitor);
  }
}
