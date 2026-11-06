#include <cstdint>

#include "TankDrivetrain.hpp"
#include "RedRobotMotorController.hpp"
#include "ultrasonic.hpp"

// Drivetrain Pin Definitions

// TODO: Find pin values on real hardware
constexpr uint8_t L_FORWARD_HIGH_PIN = 2;
constexpr uint8_t L_BACKWARD_HIGH_PIN = 3;

constexpr uint8_t R_FORWARD_HIGH_PIN = 4;
constexpr uint8_t R_BACKWARD_HIGH_PIN = 5;

constexpr uint8_t L_PWM = 9;
constexpr uint8_t R_PWM = 10;

// Sensor Pin Definitions
constexpr uint8_t ULTRASONIC_TRIG_PIN = 6;
constexpr uint8_t ULTRASONIC_ECHO_PIN = 7;
constexpr long OBSTACLE_DISTANCE_CM = 30;
constexpr unsigned long SENSOR_PERIOD_MS = 60;

RedRobotMotorController leftMotor = RedRobotMotorController(L_FORWARD_HIGH_PIN, L_BACKWARD_HIGH_PIN, L_PWM);
RedRobotMotorController rightMotor = RedRobotMotorController(R_FORWARD_HIGH_PIN, R_BACKWARD_HIGH_PIN, R_PWM);

TankDrivetrain drivetrain = TankDrivetrain(leftMotor, rightMotor);

void setup()
{
    Serial.begin(9600);
    Serial.println();
    Serial.println("Starting...");

    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(ULTRASONIC_TRIG_PIN, OUTPUT);
    pinMode(ULTRASONIC_ECHO_PIN, INPUT);
    digitalWrite(ULTRASONIC_TRIG_PIN, LOW);

    for(int i = 0; i < 10; i++){
        digitalWrite(LED_BUILTIN, LOW);
        delay(200);
        digitalWrite(LED_BUILTIN, HIGH);
        delay(200);
    }

    delay(2000);
}

/*
This function is currently very naive. It:
- Drives forward for 1 millisecond
    - If it sees an obstacle, it does a preplanned avoidance routine
    - If durationMs has elapsed, it stops
*/
void driveAndAvoidObstacles(int leftVelocity, int rightVelocity,
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

    drivetrain.runOpenLoop(-160, 160);
    delay(500);

    drivetrain.runOpenLoop(180, 180);
    delay(900);

    drivetrain.runOpenLoop(160, -160);
    delay(500);

    drivetrain.runOpenLoop(180, 180);
    delay(1800);

    drivetrain.runOpenLoop(160, -160);
    delay(500);

    drivetrain.runOpenLoop(180, 180);
    delay(900);

    drivetrain.stop();
}

void loop()
{
    Serial.println("Driving down straightaway");
    driveAndAvoidObstacles(255, 255, 3000);

    Serial.println("Driving around corner");
    driveAndAvoidObstacles(100, 255, 6000);

    drivetrain.stop();
    Serial.println("Pausing");
    delay(1000);
}
