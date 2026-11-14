#include "ArduinoQuadratureEncoder.hpp"
#include "BenchTest.hpp"
#include "MPU6050IMU.hpp"
#include "MonitorInput.hpp"
#include "PulseCalibration.hpp"
#include "PrototypeCourse.hpp"
#include "TankDrivetrain.hpp"
#include "TeamOneMotorController.hpp"
#include <Arduino_RouterBridge.h>
#include <Wire.h>

// UNO Q header pins. Set each direction so a positive command moves the car
// forward.
constexpr uint8_t LEFT_ESC_PIN = 10;
constexpr uint8_t RIGHT_ESC_PIN = 9;
constexpr bool LEFT_ESC_REVERSED = true;
constexpr bool RIGHT_ESC_REVERSED = false;
constexpr int LEFT_ESC_NEUTRAL_US = 1450;
constexpr int RIGHT_ESC_NEUTRAL_US = 1450;
constexpr unsigned long ESC_ARM_MS =
    10000; // Allow time for ESC startup and arming.
enum class DriveMode {
  Idle, BenchTest, EncoderTest, HeadingTest, StraightTest, PulseCalibration,
  StandalonePulseTest, PrototypeCourse
};
// Select another mode for bench tests or calibration.
constexpr DriveMode DRIVE_MODE = DriveMode::PrototypeCourse;

// Raw ESC pulses for one automatic trial with the wheels raised. Set either
// side to its neutral constant to test the other wheel by itself.
constexpr int TEST_LEFT_PULSE_US = 1625; // Low forward command on the left.
constexpr int TEST_RIGHT_PULSE_US = RIGHT_ESC_NEUTRAL_US;
constexpr unsigned long TEST_PULSE_DURATION_MS = 2000;
// A flat IMU with +Z pointing up reports a positive yaw for a left turn.
constexpr bool IMU_YAW_REVERSED = false;

// Hold neutral before automatic course startup so the ESCs have time to arm.
constexpr unsigned long COURSE_START_WAIT_MS = 15000;
// Set true to validate the launch straight without attempting a turn.
constexpr bool COURSE_FIRST_STRAIGHT_ONLY = false;

constexpr float WHEEL_DIAMETER_MM = 67.0f;

// REV Through Bore encoders produce 2048 cycles per shaft revolution.
// The quadrature decoder counts all four edges in each cycle.
constexpr int32_t ENCODER_COUNTS_PER_REV = 8192;
constexpr uint8_t LEFT_ENCODER_A_PIN = 4;
constexpr uint8_t LEFT_ENCODER_B_PIN = 5;
constexpr uint8_t RIGHT_ENCODER_A_PIN = 7;
constexpr uint8_t RIGHT_ENCODER_B_PIN = 6;
constexpr bool LEFT_ENCODER_REVERSED = false;
constexpr bool RIGHT_ENCODER_REVERSED = true;

TeamOneMotorController leftMotor(LEFT_ESC_PIN, LEFT_ESC_REVERSED, LEFT_ESC_NEUTRAL_US);
TeamOneMotorController rightMotor(RIGHT_ESC_PIN, RIGHT_ESC_REVERSED, RIGHT_ESC_NEUTRAL_US);
TankDrivetrain drivetrain(leftMotor, rightMotor);
ArduinoQuadratureEncoder leftEncoder(LEFT_ENCODER_A_PIN, LEFT_ENCODER_B_PIN,
                                     ENCODER_COUNTS_PER_REV, LEFT_ENCODER_REVERSED);
ArduinoQuadratureEncoder rightEncoder(RIGHT_ENCODER_A_PIN, RIGHT_ENCODER_B_PIN,
                                      ENCODER_COUNTS_PER_REV, RIGHT_ENCODER_REVERSED);

MPU6050IMU courseImu;
PrototypeCourse course(drivetrain, leftEncoder, rightEncoder, courseImu,
                       WHEEL_DIAMETER_MM, IMU_YAW_REVERSED);

static void runSelectedTest();

void setup() {
  // Servo repeats neutral while we wait for ESC power and Monitor input.
  leftMotor.begin();
  rightMotor.begin();

  if (DRIVE_MODE == DriveMode::StandalonePulseTest) {
    delay(ESC_ARM_MS);
    leftMotor.writePulseMicroseconds(TEST_LEFT_PULSE_US);
    rightMotor.writePulseMicroseconds(TEST_RIGHT_PULSE_US);
    delay(TEST_PULSE_DURATION_MS);
    drivetrain.stop();
    return;
  }

  if (DRIVE_MODE != DriveMode::PrototypeCourse) {
    Monitor.begin();
    Monitor.println("Holding ESCs at neutral for arming.");
    runSelectedTest();
    return;
  }

  // Power the robot and align it before starting the sketch. Keep it still
  // through ESC arming and gyro calibration. The course launches once.
  delay(COURSE_START_WAIT_MS);
  Wire.begin();
  courseImu.setLoggingEnabled(false);
  if (course.initializeSensors()) {
    course.start(COURSE_FIRST_STRAIGHT_ONLY);
  }
}

void loop() {
  if (DRIVE_MODE != DriveMode::PrototypeCourse) {
    delay(100);
    return;
  }

  const unsigned long now = millis();
  float yawDegrees;
  if (course.sampleHeadingError(now, yawDegrees)) {
    switch (course.state()) {
    case PrototypeCourse::CourseState::Straight:
      course.updateStraight(now, yawDegrees);
      break;
    case PrototypeCourse::CourseState::Turn:
      course.updateTurn(now, yawDegrees);
      break;
    case PrototypeCourse::CourseState::Settle:
      course.updateSettle(now, yawDegrees);
      break;
    case PrototypeCourse::CourseState::Idle:
      break;
    }
  }
  delay(1);
}

static bool initializeTestEncoders() {
  const bool leftReady = leftEncoder.begin();
  const bool rightReady = rightEncoder.begin();
  Monitor.println(leftReady ? "Left encoder initialized." : "Left encoder initialization failed.");
  Monitor.println(rightReady ? "Right encoder initialized." : "Right encoder initialization failed.");
  return leftReady && rightReady;
}

// Dispatch the optional bench modes selected at the top of the sketch.
static void runSelectedTest() {
  // Heading and straight tests have their own Monitor start commands.
  if (DRIVE_MODE == DriveMode::HeadingTest || DRIVE_MODE == DriveMode::StraightTest) {
    if (DRIVE_MODE == DriveMode::StraightTest && !initializeTestEncoders()) {
      return;
    }

    Wire.begin();
    const bool imuReady = courseImu.begin();
    if (DRIVE_MODE == DriveMode::StraightTest) {
      // Encoder-only comparisons still work if the gyro is unavailable.
      runStraightBenchTest(drivetrain, leftEncoder, rightEncoder,
                           imuReady ? &courseImu : nullptr, IMU_YAW_REVERSED);
    } else if (imuReady) {
      runHeadingBenchTest(drivetrain, courseImu, IMU_YAW_REVERSED);
    }
    return;
  }

  waitForMonitorMessage(Monitor, "Waiting for a monitor message to continue...");
  switch (DRIVE_MODE) {
  case DriveMode::PulseCalibration:
    Monitor.println("Sending neutral to both ESCs. Power on the robot and let the ESCs arm.");
    delay(ESC_ARM_MS);
    discardMonitorInput(Monitor);
    runPulseCalibration(Monitor, leftMotor, rightMotor);
    break;
  case DriveMode::EncoderTest:
    if (initializeTestEncoders()) {
      runEncoderBenchTest(drivetrain, leftEncoder, rightEncoder);
    }
    break;
  case DriveMode::BenchTest:
    runBenchTest(drivetrain);
    break;
  default:
    break;
  }
}
