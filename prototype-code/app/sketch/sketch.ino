#include "ArduinoQuadratureEncoder.hpp"
#include "BenchTest.hpp"
#include "MPU6050IMU.hpp"
#include "MonitorInput.hpp"
#include "PulseCalibration.hpp"
#include "TankDrivetrain.hpp"
#include "TeamOneMotorController.hpp"
#include "ultrasonic.hpp"
#include <Arduino_RouterBridge.h>
#include <Wire.h>
#include <math.h>

// UNO Q header pins. Set each direction so a positive command moves the car
// forward.
constexpr uint8_t LEFT_ESC_PIN = 11;
constexpr uint8_t RIGHT_ESC_PIN = 9;
constexpr bool LEFT_ESC_REVERSED = true;
constexpr bool RIGHT_ESC_REVERSED = false;
constexpr int LEFT_ESC_NEUTRAL_US = 1450;
constexpr int RIGHT_ESC_NEUTRAL_US = 1450;
constexpr unsigned long ESC_ARM_MS =
    5000; // Allow time for ESC startup and arming.
constexpr unsigned int ESC_ARM_BLINK_WAIT_MS = 100;

// Pulses that gave ~600 deg/s (~28 in/s on a 69 mm radius wheel) in the air
// tests
const int A_PLUS = 1597, A_MINUS = 1376;
const int B_PLUS = 1594, B_MINUS = 1370;

enum class DriveMode {
  Idle, BenchTest, EncoderTest, HeadingTest, StraightTest, PulseCalibration,
  PrototypeCourse
};
constexpr DriveMode DRIVE_MODE = DriveMode::StraightTest;
// A flat IMU with +Z pointing up reports a positive yaw for a left turn.
constexpr bool IMU_YAW_REVERSED = false;

// Sensor Pin Definitions
constexpr uint8_t ULTRASONIC_TRIG_PIN = 2;
constexpr uint8_t ULTRASONIC_ECHO_PIN = 3;
constexpr long OBSTACLE_DISTANCE_CM = 30;
constexpr unsigned long SENSOR_PERIOD_MS = 60;

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

void setup() {
  // Servo repeats neutral while we wait for ESC power and Monitor input.
  leftMotor.begin();
  rightMotor.begin();
  Monitor.begin();

  Monitor.println("Starting...");

  // AM32 keeps drive disabled at startup until it sees a steady neutral signal.
  // The Servo library keeps sending neutral pulses during this wait.
  drivetrain.stop();
  Monitor.println("Holding ESCs at neutral for arming.");
  if (DRIVE_MODE == DriveMode::StraightTest) {
    const bool leftReady = leftEncoder.begin();
    const bool rightReady = rightEncoder.begin();
    if (!leftReady || !rightReady) {
      Monitor.println("Straight test unavailable: both encoders must initialize.");
      return;
    }
    Wire.begin();
    MPU6050IMU imu;
    const bool imuReady = imu.begin();
    runStraightBenchTest(drivetrain, leftEncoder, rightEncoder,
                         imuReady ? &imu : nullptr, IMU_YAW_REVERSED);
    return;
  }
  if (DRIVE_MODE == DriveMode::HeadingTest) {
    Wire.begin();
    MPU6050IMU imu;
    if (imu.begin()) {
      runHeadingBenchTest(drivetrain, imu, IMU_YAW_REVERSED);
    }
    return;
  }
  waitForMonitorMessage(Monitor, "Waiting for a monitor message to continue...");
  // for (unsigned int i = 0; i < ESC_ARM_MS;) {
  //   digitalWrite(LED_BUILTIN, LOW);
  //   delay(ESC_ARM_BLINK_WAIT_MS);
  //   digitalWrite(LED_BUILTIN, HIGH);
  //   delay(ESC_ARM_BLINK_WAIT_MS);
  //   i += ESC_ARM_BLINK_WAIT_MS * 2;
  // }

  const bool leftEncoderReady = leftEncoder.begin();
  const bool rightEncoderReady = rightEncoder.begin();

  Monitor.println(leftEncoderReady ? "Left encoder initialized." : "Left encoder initialization failed.");
  Monitor.println(rightEncoderReady ? "Right encoder initialized." : "Right encoder initialization failed.");
  if (!leftEncoderReady || !rightEncoderReady) {
    // while (true) {
    //   delay(1000);
    // }
  }

  if (DRIVE_MODE == DriveMode::PulseCalibration) {
    Monitor.println("Sending neutral to both ESCs. Power on the robot and let the ESCs arm.");
    delay(ESC_ARM_MS);
    discardMonitorInput(Monitor);
    runPulseCalibration(Monitor, leftMotor, rightMotor);
  } else if (DRIVE_MODE == DriveMode::EncoderTest) {
    runEncoderBenchTest(drivetrain, leftEncoder, rightEncoder);
  } else if (DRIVE_MODE == DriveMode::BenchTest) {
    runBenchTest(drivetrain);
  } else if (DRIVE_MODE == DriveMode::PrototypeCourse) {
    // pinMode(ULTRASONIC_TRIG_PIN, OUTPUT);
    // pinMode(ULTRASONIC_ECHO_PIN, INPUT);
    // digitalWrite(ULTRASONIC_TRIG_PIN, LOW);
  }
}

/*
This function is currently very naive. It:
- Drives forward for 1 millisecond
    - If it sees an obstacle, it does a preplanned avoidance routine
    - If durationMs has elapsed, it stops
*/
void driveAndAvoidObstacles(float leftVelocity, float rightVelocity,
                            unsigned long durationMs) {
  const unsigned long endTime = millis() + durationMs;
  unsigned long lastSensorRead = 0;

  while (millis() < endTime) {
    // drivetrain.runOpenLoop(leftVelocity, rightVelocity);

    const unsigned long now = millis();
    if (now - lastSensorRead >= SENSOR_PERIOD_MS) {
      lastSensorRead = now;

      const long distanceCm =
          readDistanceCm(ULTRASONIC_TRIG_PIN, ULTRASONIC_ECHO_PIN);
      if (distanceCm > 0 && distanceCm <= OBSTACLE_DISTANCE_CM) {
        // TODO: Test IRL and figure out how to account for avoiding the
        // obstacle in the path
        driveAroundObstacle();
      }
    }
    delay(1);
  }

  // drivetrain.stop();
}

void driveAroundObstacle() {
  // drivetrain.stop();
  delay(150);

  // drivetrain.runOpenLoop(-0.5, 0.5);
  delay(500);

  // drivetrain.runOpenLoop(0.5, 0.5);
  delay(900);

  // drivetrain.runOpenLoop(0.5, -0.5);
  delay(500);

  // drivetrain.runOpenLoop(0.5, 0.5);
  delay(1800);

  // drivetrain.runOpenLoop(0.5, -0.5);
  delay(500);

  // drivetrain.runOpenLoop(0.5, 0.5);
  delay(900);

  // drivetrain.stop();
}

void loop() {
  if (DRIVE_MODE != DriveMode::PrototypeCourse) {
    delay(100);
    return;
  }

  Monitor.println("Driving down straightaway");
  //   driveAndAvoidObstacles(0.5, 0.5, 3000);
  drivetrain.runOpenLoop(0.3, 0.3);
  delay(3000);

  // Monitor.println("Driving around corner");
  // //   driveAndAvoidObstacles(0.25, 0.5, 6000);
  // drivetrain.runOpenLoop(0.25, 0.5);
  // delay(6000);

  // drivetrain.stop();
  // Monitor.println("Pausing");
  // delay(1000);
}
