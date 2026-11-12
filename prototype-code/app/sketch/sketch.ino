#include <Arduino_RouterBridge.h>
#include <Servo.h>

Servo escA; // pin 11
Servo escB; // pin 9

const int PIN_A = 11;
const int PIN_B = 9;
const int NEUTRAL = 1500;

// Pulses that gave ~600 deg/s (~28 in/s on a 69 mm radius wheel) in the air
// tests
const int A_PLUS = 1597, A_MINUS = 1376;
const int B_PLUS = 1594, B_MINUS = 1370;

// Which pulse side is robot-forward for each ESC (motors are mounted mirrored).
// If a wheel drives the wrong way on the first test, flip its flag.
const bool A_FWD_IS_PLUS = true;
const bool B_FWD_IS_PLUS = false;

const int PIVOT_SIGN = +1; // flip if the robot turns the wrong way at corners

// Timing: tune these on the real field
const int LONG_MS = 2570; // ~73 in at ~28 in/s
const int SHORT_MS = 880; // ~25 in
const int TURN_MS = 230;  // 90-degree pivot
const int PAUSE_MS = 150; // settle between segments
const int LAPS = 1;
const int ARM_SECONDS = 5; // ESC arming time at neutral
const int POLL_MS = 10;    // how often to check for a stop key during motion

int pulseA(int dir) { // dir: +1 robot-forward, -1 robot-backward, 0 stop
  if (dir == 0)
    return NEUTRAL;
  bool plus = ((dir > 0) == A_FWD_IS_PLUS);
  return plus ? A_PLUS : A_MINUS;
}

int pulseB(int dir) {
  if (dir == 0)
    return NEUTRAL;
  bool plus = ((dir > 0) == B_FWD_IS_PLUS);
  return plus ? B_PLUS : B_MINUS;
}

void flushInput() {
  while (Monitor.available() > 0) {
    Monitor.read();
  }
}

void stopMotors() {
  escA.writeMicroseconds(NEUTRAL);
  escB.writeMicroseconds(NEUTRAL);
}

// Replacement for delay(): checks for a keypress every POLL_MS.
// Returns true if the full time elapsed, false if a stop key was received
// (motors are already at neutral when it returns false).
bool waitMs(int ms) {
  unsigned long t0 = millis();
  while (millis() - t0 < (unsigned long)ms) {
    if (Monitor.available() > 0) {
      stopMotors();
      return false;
    }
    delay(POLL_MS);
  }
  return true;
}

// Returns false if stopped by key
bool drive(int dirA, int dirB, int ms) {
  escA.writeMicroseconds(pulseA(dirA));
  escB.writeMicroseconds(pulseB(dirB));
  bool ok = waitMs(ms);
  stopMotors();
  if (!ok)
    return false;
  return waitMs(PAUSE_MS);
}

// Returns true if all laps completed, false if stopped by key
bool runLaps() {
  for (int lap = 0; lap < LAPS; lap++) {
    for (int c = 0; c < 4; c++) {
      if (!drive(+1, +1, (c % 2 == 0) ? LONG_MS : SHORT_MS)) // straight
        return false;
      if (!drive(PIVOT_SIGN, -PIVOT_SIGN, TURN_MS)) // corner
        return false;
    }
    Monitor.println("Lap done");
  }
  return true;
}

void waitForKey() {
  // Let any trailing newline from a previous key arrive, then discard
  // everything so a stale key can't start (or stop) the run
  delay(300);
  flushInput();
  Monitor.println("Ready. Reposition the robot, then send any key to start.");
  while (Monitor.available() <= 0) {
    delay(10);
  }
  // Let a trailing newline arrive, then discard it so it can't act as a stop
  // key
  delay(100);
  flushInput();
}

void setup() {
  Monitor.begin(115200);
  escA.attach(PIN_A, 1000, 2000);
  escB.attach(PIN_B, 1000, 2000);
  stopMotors();
  Monitor.println("Neutral. Power the ESCs now.");

  // ESC arming countdown (neutral held). Only needed once per power-up;
  // the ESCs stay armed at neutral between runs.
  for (int i = ARM_SECONDS; i > 0; i--) {
    Monitor.println(i);
    delay(1000);
  }
}

void loop() {
  stopMotors(); // hold neutral while waiting
  waitForKey();

  Monitor.println("Go (send any key to stop)");
  if (runLaps()) {
    Monitor.println("Finished");
  } else {
    Monitor.println("STOPPED by key.");
  }
  // loop() repeats: back to the ready prompt
}