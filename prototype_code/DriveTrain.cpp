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



void DriveTrain::Forward(int speed){
  M_Right.Motor_start(speed);
  M_Left.Motor_start(speed);
}

void DriveTrain::Backward(int speed){
  M_Right.Motor_start(-speed);
  M_Left.Motor_start(-speed);
}

void DriveTrain::Right(int speed, double stepdown){
  M_Right.Motor_start(speed * stepdown);
  M_Left.Motor_start(speed);
}

void DriveTrain::Left(int speed, double stepdown){
  M_Right.Motor_start(speed);
  M_Left.Motor_start(speed * stepdown);
}

void DriveTrain::Clockwise(int speed){
  M_Right.Motor_start(-speed);
  M_Left.Motor_start(speed);
}

void DriveTrain::Counterclockwise(int speed){
  M_Right.Motor_start(speed);
  M_Left.Motor_start(-speed);
}

void DriveTrain::TestMotor(MotorControl test, int speed){
  stop();
  test.Motor_start(speed);
  delay(1000);
  stop();
}

void DriveTrain::TestMotors(int speed){
  TestMotor(M_Left, speed);
  TestMotor(M_Left, -speed);
  TestMotor(M_Right, speed);
  TestMotor(M_Left, -speed);
}

void DriveTrain::TestDrive(int speed){
  Forward(speed);
  Backward(speed);
  Right(speed, 0.95);
  Left(speed, 0.95);
  Clockwise(speed);
  Counterclockwise(speed);
}

void DriveTrain::PrintThing(String name, double value){
  Serial.print(" " + name + ": ");
  Serial.print(value);
}
