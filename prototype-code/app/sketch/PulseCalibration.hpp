#pragma once

#include <Arduino.h>

class TeamOneMotorController;

void runPulseCalibration(Stream &monitor, TeamOneMotorController &left,
                         TeamOneMotorController &right);
