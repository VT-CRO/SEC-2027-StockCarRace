#include "BenchTest.hpp"
#include "TankDrivetrain.hpp"
#include "TeamOneMotorController.hpp"
#include "ultrasonic.hpp"

// UNO Q header pins. Set each direction so a positive command moves the car forward.
constexpr uint8_t LEFT_ESC_PIN = 9;
constexpr uint8_t RIGHT_ESC_PIN = 10;
constexpr bool LEFT_ESC_REVERSED = false;
constexpr bool RIGHT_ESC_REVERSED = true; // Initial guess for mirrored mounting.
constexpr unsigned long ESC_ARM_MS = 3000; // Allow time for ESC startup and arming.

enum class DriveMode { Idle, BenchTest, PrototypeCourse };
constexpr DriveMode DRIVE_MODE = DriveMode::Idle;

// Sensor Pin Definitions
constexpr uint8_t ULTRASONIC_TRIG_PIN = 6;
constexpr uint8_t ULTRASONIC_ECHO_PIN = 7;
constexpr long OBSTACLE_DISTANCE_CM = 30;
constexpr unsigned long SENSOR_PERIOD_MS = 60;

TeamOneMotorController leftMotor(LEFT_ESC_PIN, LEFT_ESC_REVERSED);
TeamOneMotorController rightMotor(RIGHT_ESC_PIN, RIGHT_ESC_REVERSED);
TankDrivetrain drivetrain(leftMotor, rightMotor);

void setup()
{
    Serial.begin(9600);
    Serial.println();
    Serial.println("Starting...");
    leftMotor.begin();
    rightMotor.begin();

    Serial.println("Holding ESCs at neutral for arming.");
    // AM32 keeps drive disabled at startup until it sees a steady neutral signal.
    // The Servo library keeps sending neutral pulses during this wait.
    delay(ESC_ARM_MS);

    if (DRIVE_MODE == DriveMode::BenchTest) {
        runBenchTest(drivetrain);
    } else if (DRIVE_MODE == DriveMode::PrototypeCourse) {
        pinMode(ULTRASONIC_TRIG_PIN, OUTPUT);
        pinMode(ULTRASONIC_ECHO_PIN, INPUT);
        digitalWrite(ULTRASONIC_TRIG_PIN, LOW);
    }

    pinMode(LED_BUILTIN, OUTPUT);
    for(int i = 0; i < 10; i++){
        digitalWrite(LED_BUILTIN, LOW);
        delay(200);
        digitalWrite(LED_BUILTIN, HIGH);
        delay(200);
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
        drivetrain.runOpenLoop(leftVelocity, rightVelocity);

        const unsigned long now = millis();
        if (now - lastSensorRead >= SENSOR_PERIOD_MS) {
            lastSensorRead = now;

            const long distanceCm =
                readDistanceCm(ULTRASONIC_TRIG_PIN, ULTRASONIC_ECHO_PIN);
            if (distanceCm > 0 && distanceCm <= OBSTACLE_DISTANCE_CM) {
                // TODO: Test IRL and figure out how to account for avoiding the obstacle in the path
                driveAroundObstacle();
            }
        }
        delay(1);
    }

    drivetrain.stop();
}

void driveAroundObstacle() {
    drivetrain.stop();
    delay(150);

    drivetrain.runOpenLoop(-0.5, 0.5);
    delay(500);

    drivetrain.runOpenLoop(0.5, 0.5);
    delay(900);

    drivetrain.runOpenLoop(0.5, -0.5);
    delay(500);

    drivetrain.runOpenLoop(0.5, 0.5);
    delay(1800);

    drivetrain.runOpenLoop(0.5, -0.5);
    delay(500);

    drivetrain.runOpenLoop(0.5, 0.5);
    delay(900);

    drivetrain.stop();
}

void loop()
{
    if (DRIVE_MODE != DriveMode::PrototypeCourse) {
        delay(100);
        return;
    }

    Serial.println("Driving down straightaway");
    driveAndAvoidObstacles(0.5, 0.5, 3000);

    Serial.println("Driving around corner");
    driveAndAvoidObstacles(0.25, 0.5, 6000);

    drivetrain.stop();
    Serial.println("Pausing");
    delay(1000);
}
