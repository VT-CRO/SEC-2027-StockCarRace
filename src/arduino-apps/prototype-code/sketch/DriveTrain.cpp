#include "MotorControl.h"
#include "DriveTrain.h"
#include "MPU6050IO.h"
#include "Macros.h"
#include <math.h>
#include <Arduino_RouterBridge.h>


#ifndef PI
#define PI 3.14159265f
#endif

#define speedForDist 200
#define speedForRot 120
#define timeToDistY 10000/72.75
#define timeToDistX 700
#define timeToDeg 2200/90

DriveTrain::DriveTrain(){}

DriveTrain::DriveTrain(MotorControl L, MotorControl R){
  M_Left = L;
  M_Right = R;
}

DriveTrain::DriveTrain(int L_1, int L_2, int L_3, int R_1, int R_2, int R_3){
  M_Left = MotorControl(L_1, L_2, L_3);
  M_Right = MotorControl(R_1, R_2, R_3);
}

void DriveTrain::stop(){
  M_Right.Motor_start(0);
  M_Left.Motor_start(0);
}

float DriveTrain::drive(float forward, float targetYaw, MPU6050IO& imu, float outputLimit) {
  MPU6050IO::Orientation orientation;
  if (!imu.readOrientation(orientation)) {
    stop();
    return NAN;
  }

  float yaw = orientation.yaw;
  float error = targetYaw - yaw;
  while (error > PI) {
    error -= 2.0f * PI;
  }
  while (error < -PI) {
    error += 2.0f * PI;
  }

  float output = YAW_P_GAIN * error;
  if (outputLimit < 0.0f) {
    outputLimit = 0.0f;
  } else if (outputLimit > 1.0f) {
    outputLimit = 1.0f;
  }
  if (output > outputLimit) {
    output = outputLimit;
  } else if (output < -outputLimit) {
    output = -outputLimit;
  }

  float left = forward + output;
  float right = forward - output;
  if (left > 1.0f) {
    left = 1.0f;
  } else if (left < -1.0f) {
    left = -1.0f;
  }
  if (right > 1.0f) {
    right = 1.0f;
  } else if (right < -1.0f) {
    right = -1.0f;
  }

  set_motor_states(left, right);
  return error;
}

void DriveTrain::set_motor_states(float left, float right){
  leftM_state = left;
  rightM_state = right;
  M_Left.Motor_start((int)(left * 255));
  M_Right.Motor_start((int)(-right * 255));
}


void DriveTrain::print_robot_states(){
  Monitor.print("Left motor state: ");
  Monitor.print(leftM_state);
  Monitor.print(", Right motor state: ");
  Monitor.println(rightM_state);
}
