#ifndef BASICDRIVE_H_
#define BASICDRIVE_H_

#include "MotorControl.h"

class DriveTrain {

public: 

DriveTrain();
DriveTrain(MotorControl L, MotorControl R);
DriveTrain(int L_1, int L_2, int L_3, int R_1, int R_2, int R_3);

void stop();

void Forward(int speed, int time);
void Backward(int speed, int time);
void Right(int speed, double stepdown, int time);
void Left(int speed, double stepdown, int time);
void Clockwise(int speed, int time);
void Counterclockwise(int speed, int time);

void TestMotor(MotorControl test, int speed, int time);
void TestMotors(int speed, int time);
void TestDrive(int speed, int time);

void PrintThing(String name, double value);

private:

MotorControl M_Left;
MotorControl M_Right;


public:

double E_Left_Prev{0};
double E_Right_Prev{0};
double E_Center_Prev{0};

double E_Left_Current{0};
double E_Right_Current{0};
double E_Center_Current{0};

double m_x{0};
double m_y{0};
double m_heading{0};

double  m_R{(double)1.6};
double  m_N{4096};
double  m_B{(double)0.0};
double  m_L{(double)22.698711};
double  m_CM_PER_TICK{(2.0*(double)PI * 1.6)/4096};

struct PID {
    double m_p;
    double m_i;
    double m_d;
};

PID m_x_PID = {.m_p = 100.0, .m_i = 0.0, .m_d = 0.0};
PID m_y_PID = {.m_p = 100.0, .m_i = 0.0, .m_d = 0.0};
PID m_heading_PID = {.m_p = 1000.0, .m_i = 0.0, .m_d = 0.0};

double  m_x_threashold{1};
double  m_y_threashold{1};
double  m_heading_threashold{0.0175};

int speed_Max{150};
int speed_Min{-150};

int FR_speed{0};
int FL_speed{0};
int BR_speed{0};
int BL_speed{0};

double Error_x{0};
double Error_y{0};
double Error_heading{0};

double Error_x_Prev{0};
double Error_y_Prev{0};
double Error_heading_Prev{0};

double Error_d_x{0.0};
double Error_d_y{0.0};
double Error_d_heading{0.0};

double Error_i_x{0.0};
double Error_i_y{0.0};
double Error_i_heading{0.0};


};

#endif