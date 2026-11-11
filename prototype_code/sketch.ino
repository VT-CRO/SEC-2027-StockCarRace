#include "MotorControl.h"
#include "DriveTrain.h"
#define ENCODER_OPTIMIZE_INTERRUPTS
#include "Macros.h"
#include "ToF.h"
#include "MPU6050IO.h"
#include <Arduino_RouterBridge.h>


MPU6050IO imu;
ToF tof;

int state = 0;
double currentTargetYaw = 0;
double currentForward = 1;


MotorControl R = MotorControl(L_PIN_1, L_PIN_2, L_PWM);
MotorControl L = MotorControl(R_PIN_1, R_PIN_2, R_PWM);


DriveTrain Robot(L, R);


void setup() {
  Wire.begin();

  Monitor.begin();

  Monitor.println();
  Monitor.println("Start");

  if (!tof.begin()) {
    while (1) {
      Monitor.println("Failed to initialize ToF sensor!");
      delay(1000);
    }
  }

  if (!imu.begin()) {
    while (1) {
      Monitor.println("Failed to initialize MPU6050 sensor!");
      delay(1000);
    }
  }



  pinMode(LED_BUILTIN, OUTPUT);
  for(int i = 0; i < 10; i++){
    digitalWrite(LED_BUILTIN, LOW);   // turn the LED off by making the voltage LOW
    delay(200);                      // wait for a second
    digitalWrite(LED_BUILTIN, HIGH);  // turn the LED on (HIGH is the voltage level)
    delay(200);                      // wait for a second
  }

  Monitor.println("Setup complete, waiting for imu calibration");
  imu.calibrate();


  while (!Monitor.available()) {
    Monitor.println("Waiting for start signal (Send any char)...");
    delay(2000);
  };

  Monitor.println("Starting...");
}

void loop() {
  double dis = tof.readDistanceCm();

  double error = Robot.drive(currentForward, currentTargetYaw, imu, 1.0f);

  switch (state) {
    case 0:
      if (dis < 30) {
        currentTargetYaw -= PI / 2;
        if (currentTargetYaw < -PI) { // wrapping back
          currentTargetYaw += 2 * PI;
        }
        currentForward = 0;
        state = 1;
      }
      break;
    case 1:
      if (abs(error) < 0.05) {
        currentForward = 1;
        state = 0;
      }
      break;
  }

  Monitor.print("Distance: ");
  Monitor.println(dis);

  delay(20);
}
