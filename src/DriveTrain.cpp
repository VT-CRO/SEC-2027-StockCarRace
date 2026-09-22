#include "MotorControl.h"
#include "DriveTrain.h"

#define speedForDist 200
#define speedForRot 120
#define timeToDistY 10000/72.75
#define timeToDistX 700
#define timeToDeg 2200/90

DriveTrain::DriveTrain(){

}

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

void DriveTrain::Forward(int speed, int time){
  M_Right.Motor_start(speed);
  M_Left.Motor_start(speed);

  delay(time);
  stop();
}

void DriveTrain::Backward(int speed, int time){
  M_Right.Motor_start(-speed);
  M_Left.Motor_start(-speed);

  delay(time);
  stop();
}

void DriveTrain::Right(int speed, double stepdown, int time){
  M_Right.Motor_start(speed * stepdown);
  M_Left.Motor_start(speed);

  delay(time);
  stop();
}

void DriveTrain::Left(int speed, double stepdown, int time){
  M_Right.Motor_start(speed);
  M_Left.Motor_start(speed * stepdown);

  delay(time);
  stop();
}

void DriveTrain::Clockwise(int speed, int time){
  M_Right.Motor_start(-speed);
  M_Left.Motor_start(speed);

  delay(time);
  stop();
}

void DriveTrain::Counterclockwise(int speed, int time){
  M_Right.Motor_start(speed);
  M_Left.Motor_start(-speed);

  delay(time);
  stop();
}

void DriveTrain::TestMotor(MotorControl test, int speed, int time){
  stop();
  test.Motor_start(speed);
  delay(time);
  stop();
}

void DriveTrain::TestMotors(int speed, int time){
  TestMotor(M_Left, speed, time);
  TestMotor(M_Left, -speed, time);
  TestMotor(M_Right, speed, time);
  TestMotor(M_Left, -speed, time);
}

void DriveTrain::TestDrive(int speed, int time){
  Forward(speed, time);
  Backward(speed, time);
  Right(speed, 0.95, time);
  Left(speed, 0.95, time);
  Clockwise(speed, time);
  Counterclockwise(speed, time);
}

void DriveTrain::PrintThing(String name, double value){
  Serial.print(" " + name + ": ");
  Serial.print(value);
}
