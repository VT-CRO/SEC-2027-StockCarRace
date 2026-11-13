#include "PulseCalibration.hpp"
#include "MonitorInput.hpp"
#include "TeamOneMotorController.hpp"
#include <string.h>

namespace {
constexpr unsigned long PULSE_TRIAL_MS = 5000;

int parsePulse(const char *command) {
  if (strlen(command) != 6 || (command[0] != 'l' && command[0] != 'r') ||
      command[1] != ' ') {
    return 0;
  }
  int pulseUs = 0;
  for (int i = 2; i < 6; ++i) {
    if (command[i] < '0' || command[i] > '9') {
      return 0;
    }
    pulseUs = pulseUs * 10 + command[i] - '0';
  }
  return pulseUs;
}

} // namespace

void runPulseCalibration(Stream &monitor, TeamOneMotorController &left,
                         TeamOneMotorController &right) {
  left.stop();
  right.stop();
  monitor.println("Ready. Holding neutral on both ESCs.");
  monitor.println("After the ESCs arm, send l 1625 or r 1625 to test movement.");
  monitor.println("Send stop to return both ESCs to neutral.");

  MonitorCommandReader reader(monitor);
  bool trialActive = false;
  unsigned long trialStarted = 0;

  while (true) {
    const auto result = reader.poll();
    if (result == MonitorCommandReader::Result::Pending) {
      if (trialActive && millis() - trialStarted >= PULSE_TRIAL_MS) {
        left.stop();
        right.stop();
        trialActive = false;
        monitor.println("Trial ended. Both ESCs at baseline neutral.");
      }
      delay(5);
      continue;
    }

    left.stop();
    right.stop();
    trialActive = false;
    const char *command = reader.command();
    const bool valid = result == MonitorCommandReader::Result::Command;
    const int pulseUs = valid ? parsePulse(command) : 0;
    if (valid && strcmp(command, "stop") == 0) {
      monitor.println("Both ESCs at baseline neutral.");
    } else if (pulseUs >= TeamOneMotorController::MIN_PULSE_US &&
               pulseUs <= TeamOneMotorController::MAX_PULSE_US) {
      monitor.print(command);
      monitor.println(" us accepted for 5 seconds.");
      TeamOneMotorController &motor = command[0] == 'l' ? left : right;
      motor.writePulseMicroseconds(pulseUs);
      trialStarted = millis();
      trialActive = true;
    } else {
      monitor.println("Use l 1625, r 1625, or stop. Pulse range: 1000-2000 us.");
    }
  }
}
