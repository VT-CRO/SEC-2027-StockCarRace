#include "MotorControl.h"
#include "DriveTrain.h"
#define ENCODER_OPTIMIZE_INTERRUPTS
#include "Macros.h"
#include "ServoPositions.h"
#include <Servo.h>
#include <Wire.h>
#include <VL53L0X.h>
#include <Arduino_RouterBridge.h>

VL53L0X tof; // TOF sensor

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

  Monitor.begin();

  Monitor.println();
  Monitor.println("Start");

  Wire.begin(); // SDA: 18/A4 SCL: 19/A5

  tof.setTimeout(500);
  if (!tof.init())
  {
    while (1) {
       Monitor.println("Failed to detect and initialize sensor!");
      delay(1000);
    }
  }

  tof.startContinuous();

  
  pinMode(LED_BUILTIN, OUTPUT);
  for(int i = 0; i < 10; i++){
    digitalWrite(LED_BUILTIN, LOW);   // turn the LED off by making the voltage LOW
    delay(200);                      // wait for a second
    digitalWrite(LED_BUILTIN, HIGH);  // turn the LED on (HIGH is the voltage level)
    delay(200);                      // wait for a second
  }


  Monitor.println("Waiting for start signal (Send any char)...");
  while (!Monitor.available()) {};

  Monitor.println("Starting...");

  delay(2000);

  Robot.set_motor_states(1, 1);
  
}

void loop() {
  double dis = tof.readRangeContinuousMillimeters() * 0.1; // converted to cm


  Monitor.print("Distance: ");
  Monitor.println(dis);
  if (dis < 10){
      Monitor.println("Detected!");
      Robot.stop();

      // Drives around the obstacle
      Robot.set_motor_states(-1,1);
      delay(500);
      Robot.set_motor_states(1,1);
      delay(500);
      Robot.set_motor_states(1, -1);
      delay(500);
      Robot.set_motor_states(1,1);
      delay(600);
      Robot.set_motor_states(1,-1);
      delay(300);
      Robot.set_motor_states(1,1);
      delay(500);
      Robot.set_motor_states(-1, 1);

      delay(100);
      Robot.set_motor_states(1, 1);
  }

  delay(20);
}
