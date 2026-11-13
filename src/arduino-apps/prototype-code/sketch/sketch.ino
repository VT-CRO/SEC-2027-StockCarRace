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
int turnCount = 0;
int triggerReadings = 0;
double currentTargetYaw = 0;
double currentForward = 1;


MotorControl L = MotorControl(L_PIN_1, L_PIN_2, L_PWM);
MotorControl R = MotorControl(R_PIN_1, R_PIN_2, R_PWM);


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

  // Monitor.println("Setup complete, waiting for imu calibration");
  imu.calibrate();


  while (!Monitor.available()) {
    Monitor.println("Waiting for start signal (Send any char)...");
    delay(2000);
  };

  imu.reset();

  delay(100);

  Monitor.println("Starting...");
}

void loop() {
  double dis = tof.readDistanceCm();

  // Monitor.println("help me");

  double error = Robot.drive(currentForward, currentTargetYaw, imu, 1.0f);
  
  // Robot.print_robot_states();
  // Robot.set_motor_states(1, 1);
  // R.Motor_start(255);
  // L.Motor_start(255);

   
  float triggerDis = 10.0f;

  switch (state) {
    case 0:
      if (turnCount % 4 == 2) {
        triggerDis = 80.0f;
      }

      if (dis < triggerDis) {
        if (++triggerReadings >= 3) {
          triggerReadings = 0;
          turnCount++;
          currentTargetYaw -= PI / 2;
          if (currentTargetYaw < -PI) { // wrapping back
            currentTargetYaw += 2 * PI;
          }
          currentForward = 0;
          state = 1;
          Monitor.print("State: ");
          Monitor.print(state);
          Monitor.print(" Distance: " );
          Monitor.println(dis);
        }
      } else {
        triggerReadings = 0;
      }
      break;
    case 1:
      if (abs(error) < 0.05) {
        currentForward = 1;
        state = 0;
      }
      break;
  }
  // Monitor.print(" Distance: " );
  // Monitor.println(dis);
  
  // Monitor.print("Heading error: ");
  // Monitor.println(error * (180.0f/PI));

  delay(20);
}
