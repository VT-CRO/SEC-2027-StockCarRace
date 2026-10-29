/**
 * @file MotorControl.h
 * @author Domenic Marcelli R&D Team 
 * @brief This the header file for the motor controls.
 * @date 2023-10-27
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#ifndef MOTORCONTROL_H_
#define MOTORCONTROL_H_


// #include "arduino_freertos.h"
#include "Arduino.h"
#include <avr/io.h>
#include <avr/pgmspace.h>

#define tskPID_PRIORITY 7 // can be changed. Up for discussion

#define PID_LOOP_PERIOD 50

/**
 * @brief Class representing a Motor
 */
class MotorControl {

public:

  /**
   * @brief Construct a new Motor Control object
   * 
   * @param pin1 pin 1 of motor controller
   * @param pin2 pin 2 of motor controller
   * @param pin3 pin 3 of motor controller (PWM)
   */
  MotorControl(int pin1, int pin2, int pin3);

  MotorControl();

  /**
   * @brief Set up motor PID controller
   * 
   * @param P Proportional gain
   * @param I Integral gain
   * @param D Derivative gain
   */
  void Motor_setPIDParams(float P, float I, float D);

  /**
   * @brief Enabled motor speed control
   */
  void Motor_enablePIDTask();

  /**
   * @brief Enables motors to begin running at a speed setpoint
   * 
   * @param newSpeed Desired motor speed
   */
  void Motor_start(int newSpeed);

  /**
   * @brief Initialize motor pins
   * 
   */
  void Motor_pin_init();

  // task for freeRTOS
  static void pid_task(void *pidParams);

  /**
   * @brief Motor PID loop function
   */
  void Motor_pidControlLoop();

  /**
   * @brief Get the motor speed
   * 
   * @return motor speed as an int
   */
  int getSpeed();

  /**
   * @brief Logs motor state to ROSSerial
   * 
   * @param nh The current ROS node handler
   */
 // void logState(ros::NodeHandle &nh);

private:

  /**
   * @brief Boolean indicating if Motor is running PID control
   */
  bool pidMode;

  /**
   * @brief Motor speed
   */
  int speed;

  /**
   * @brief Motor instantaneous angular velocity
   * 
   */
  int current_velocity;

  /**
   * @brief Target angular velocity
   */
  int goal_velocity;

  /**
   * @brief Internal state variable for PID
   */
  int last_error;

  /**
   * @brief Struct for holding motor pins
   */
  struct _PinAssign {
    int in1;
    int in2;
    int in3;
  };
  typedef _PinAssign PinAssign;

  /**
   * @brief Pins
   */
  PinAssign Assignments;

  /**
   * @brief Motor PID P Gain
   * 
   */
  int motorP;

  /**
   * @brief Motor PID I Gain
   * 
   */
  int motorI;

  /**
   * @brief Motor PID D Gain
   * 
   */
  int motorD;
};
// All the variables needed to control the motors

#endif
