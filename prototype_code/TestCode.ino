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

#define TRIG_PIN 0 //placeholder
#define ECHO_PIN 1 

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
  Robot.Forward(255);
}

void loop() {
  long distance = readUltrasonic();

  if (distance < 15 && distance != -1){
    Robot.stop();
    delay(100);
    Robot.Clockwise(255);
    delay(1000);
    Robot.stop();
    Robot.Forward(255);
    delay(2000);
    Robot.stop();
    Robot.Counterclockwise(255);
    delay(1000);
    Robot.stop();
    Robot.Forward(255);
    delay(2000);
    Robot.stop();
    Robot.Forward(255);
    //TS IS ASS
  }

  delay(10);
}



// Helper function to convert microseconds to centimeters
long microsecondsToCentimeters(long microseconds) {
  // Speed of sound is ~340 m/s or 29 microseconds per centimeter.
  // The pulse travels out and back, so: microseconds / 29 / 2
  return microseconds / 58;
}

long readUltrasonic() {
  // 1. Ensure a clean LOW signal first
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(4);

  // 2. Send the 10 µs trigger pulse
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // 3. Read the echo pulse with a timeout (~25,000 µs = ~4.3 meters max range)
  // Prevents the Teensy from hanging for 1 full second if the pulse misses
  long duration = pulseIn(ECHO_PIN, HIGH, 25000);

  if (duration == 0) {
    return -1; // Out of range or no echo received
  }

  // 4. Convert and return the result
  return microsecondsToCentimeters(duration);
}

