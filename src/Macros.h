#ifndef MICROS_H_
#define MICROS_H_

// PCB Micros

// Motor Drivers
#define R_PIN_1 11
#define R_PIN_2 10

#define L_PIN_1 5
#define L_PIN_2 6

#define R_PWM        9
#define L_PWM        3

// UART/I2C
#define I2C_SDA 18
#define I2C_SCL 19

// Yaw P controller: output is normalized to set_motor_states()'s -1..1 range.
#define YAW_P_GAIN 1.0f


// #define UART2_RX 25
// #define UAR2T_TX 24

// Servos
// #define PWM_1 0
// #define PWM_2 1
// #define PWM_3 2
// #define PWM_4 3
// #define PWM_5 4
// #define PWM_6 5
// #define PWM_7 6
// #define PWM_8 7
// #define PWM_9 8
// #define PWM_10 9
// #define PWM_11 10

// Relays/Solenoids
// #define SOLENOID_1 38
// #define SOLENOID_2 39
// #define SOLENOID_3 40
// #define SOLENOID_4 41

// #define Photoresistor_PIN A7
// #define LED_threashold 1000

#endif