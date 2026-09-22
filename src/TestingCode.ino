#include "MotorControl.h"
#include "DriveTrain.h"
#define ENCODER_OPTIMIZE_INTERRUPTS
#include "Macros.h"
#include "ServoPositions.h"
#include <Servo.h>

// Drivetrain Macros

#define L_PIN_1 MD1_M2_PIN_2 
#define L_PIN_2 MD1_M2_PIN_1

#define R_PIN_1 MD1_M1_PIN_1
#define R_PIN_2 MD1_M1_PIN_2

#define L_PWM   PWM_B
#define R_PWM   PWM_A

MotorControl R = MotorControl(L_PIN_1, L_PIN_2, L_PWM);
MotorControl L = MotorControl(R_PIN_1, R_PIN_2, R_PWM);


DriveTrain Robot(L, R);


void setup() {

  Serial.begin(9600);

  Serial.println();
  Serial.println("Start");

  pinMode(LED_BUILTIN, OUTPUT);
  for(int i = 0; i < 10; i++){
    digitalWrite(LED_BUILTIN, LOW);   // turn the LED off by making the voltage LOW
    delay(200);                      // wait for a second
    digitalWrite(LED_BUILTIN, HIGH);  // turn the LED on (HIGH is the voltage level)
    delay(200);                      // wait for a second
  }

  delay(2000);

}

void loop() {

  Robot.Forward(255, 5000);
  Robot.stop();
  delay(1000);
  Robot.Right(255, 0.5, 2000);
  delay(10000);
  Serial.println("hi");

}
