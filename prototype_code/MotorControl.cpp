/////////////////////////////////////////////////////////////
// Author: Domenic Marcelli   R&D Team VT CRO
// filename: MotorControl.cpp
// Last Modified: 01/14/2024
// Description:  This where the function defintions for the
//               Motor Controller are kept. Continously be updated
//
// Modifications: Changed it from handleing all the motors to just one
//                so that we can take advantage of c++ objects and threads
//                with RTOS
/////////////////////////////////////////////////////////////
#include "MotorControl.h"

// Notes:
//  Have a function that allows the encoder to listen to state of motor
//  of the encoder

// Make the functionality for the PID loop - complete?

// Motor Control Constructor
// Assigns values to the pin variables
MotorControl::MotorControl(int in1, int in2, int in3) {

  Assignments.in1 = in1;
  Assignments.in2 = in2;
  Assignments.in3 = in3;
  speed = 0;
  current_velocity = 0;
  goal_velocity = 0;
  pidMode = false;
  Motor_pin_init();
}

MotorControl::MotorControl() {

  Assignments.in1 = 0;
  Assignments.in2 = 0;
  speed = 0;
  current_velocity = 0;
  goal_velocity = 0;
  pidMode = false;
  Motor_pin_init();
}

void MotorControl::Motor_setPIDParams(float P, float I, float D) {
  motorP = P;
  motorI = I;
  motorD = D;
}

// void MotorControl::Motor_enablePIDTask() {
//   (xTaskCreate(MotorControl::pid_task, "PID control task", 100, this,
//                tskIDLE_PRIORITY + tskPID_PRIORITY, NULL) != pdTRUE);
//   pidMode = false;
// }

// Motor Speed sets the speed of the motors.
// Values go from 0 - 255 for analogWrite.
//  I want to support negative values later
//  to signify reverse.
void MotorControl::Motor_start(int newSpeed) {

  int go_pin, no_go_pin, pwm;

  pwm = Assignments.in3;

  if (newSpeed < 0) {
    go_pin = Assignments.in2;
    no_go_pin = Assignments.in1;
  } else {
    go_pin = Assignments.in1;
    no_go_pin = Assignments.in2;
  }
  speed = abs(newSpeed);
  
  digitalWrite(no_go_pin, LOW);
  digitalWrite(go_pin, HIGH);
  analogWrite(pwm, speed);
}

// void MotorControl::logState(ros::NodeHandle &nh) {
//   char buff[32];
//   sprintf(buff, "Motor Speed: %d", speed);
//   nh.loginfo(buff);
// }

// Motor_pin_init initalizates pins.
//  honestly, we might want the constuctor to handle this
void MotorControl::Motor_pin_init() {
  pinMode(Assignments.in1, OUTPUT);
  pinMode(Assignments.in2, OUTPUT);
  pinMode(Assignments.in3, OUTPUT);
}

// void MotorControl::pid_task(void *pidParams) {
//   MotorControl *instance = (MotorControl *)pidParams;

//   TickType_t ui32WakeTime = xTaskGetTickCount();

//   while (1) {
//     instance->Motor_pidControlLoop();
//     xTaskDelayUntil(&ui32WakeTime, pdMS_TO_TICKS(PID_LOOP_PERIOD));
//   }
// }

void MotorControl::Motor_pidControlLoop() {

  float error_velocity = goal_velocity - current_velocity;

  float PG = error_velocity * motorP;                // Proptional Gain
  float DG = (error_velocity - last_error) * motorD; // Differential Gain

  last_error = error_velocity;
  // need to be able to set velocity for next interation of the loop
  // right now the only thing we have is set speed which ranges from 0-256 bytes
  // figure out how that works. Probably need to deal with the encoder somewhere
  // else in the codeDo
  float controlSig = PG + DG;

  // set speed with value control Sig and convert bounds
  if (controlSig > 255) {
    controlSig = 255.0;
  } else if (controlSig < -255) {
    controlSig = -255.0;
  }

  speed = (int)controlSig;

  Motor_start(speed);
}

// void MotorControl::Motor_stopMove()
// {
//     double increment = 0.01;
//     while (increment > 0)
//     {
//         // alternatively I could just put the set speed function here
//         // but then I would need to add argurments to the function
//         analogWrite(go_pin, increment * speed);
//         delay(1);
//         increment = increment - .01;
//         Serial.print(increment);
//     }
// }

// void MotorControl::checkDirection(int newSpeed)
// {

// }

int MotorControl::getSpeed() { return speed; }