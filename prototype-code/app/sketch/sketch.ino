#include <cstdint>

#include "TankDrivetrain.hpp"
#include "RedRobotMotorController.hpp"

// Drivetrain Pin Definitions

// TODO: Find pin values on real hardware
constexpr uint8_t L_FORWARD_HIGH_PIN = 2;
constexpr uint8_t L_BACKWARD_HIGH_PIN = 3;

constexpr uint8_t R_FORWARD_HIGH_PIN = 4;
constexpr uint8_t R_BACKWARD_HIGH_PIN = 5;

constexpr uint8_t L_PWM = 9;
constexpr uint8_t R_PWM = 10;

RedRobotMotorController leftMotor = RedRobotMotorController(L_FORWARD_HIGH_PIN, L_BACKWARD_HIGH_PIN, L_PWM);
RedRobotMotorController rightMotor = RedRobotMotorController(R_FORWARD_HIGH_PIN, R_BACKWARD_HIGH_PIN, R_PWM);

TankDrivetrain drivetrain = TankDrivetrain(leftMotor, rightMotor);

void setup()
{
    Serial.begin(9600);
    Serial.println();
    Serial.println("Starting...");

    pinMode(LED_BUILTIN, OUTPUT);
    for(int i = 0; i < 10; i++){
        digitalWrite(LED_BUILTIN, LOW);
        delay(200);
        digitalWrite(LED_BUILTIN, HIGH);
        delay(200);
    }

    delay(2000);
}

void loop()
{
    // TODO: Break up long drives in code to look for obstacles
    Serial.println("Driving down straightaway");
    drivetrain.runOpenLoop(255, 255);
    delay(3000);
    Serial.println("Driving around corner");
    drivetrain.runOpenLoop(100, 255);
    delay(6000);
    drivetrain.stop();
    Serial.println("Pausing");
    delay(1000);
}
