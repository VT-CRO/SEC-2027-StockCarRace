# Team 1 Prototype Code

This folder contains the Arduino code for our prototype. It's entirely separate from the surrounding ROS2 project.

```
prototype-code/
├── app/                  # The Arduino App Lab app (app.yaml, python/, sketch/, ...)
└── scripts/
    └── unoq-sync.sh      # Deploys app/ to the Uno Q, or pulls changes back
```

## Setup (once per person)

You work on the code on your laptop and push it to the Uno Q over SSH. **The repo does not need to be cloned on the Uno Q**, and it can be cloned anywhere on your laptop. The script finds the app relative to its own location.

1. **Tools on your laptop (WSL/Linux):**
   ```bash
   sudo apt install rsync openssh-client
   ```
2. **Uno Q:** complete the normal first-time setup in Arduino App Lab, connect the board to Wi-Fi, make sure SSH is enabled, and install `rsync` in an App Lab terminal:
   ```bash
   sudo apt update && sudo apt install -y rsync
   ```
3. **First run only:** make the script executable if it isn't already:
   ```bash
   chmod +x prototype-code/scripts/unoq-sync.sh
   ```

## Deploy (laptop → Uno Q)

From anywhere inside the repo:

```bash
./prototype-code/scripts/unoq-sync.sh deploy
```

What it does:

1. Finds the board's IP (see below).
2. Stops the app on the board (`arduino-app-cli app stop`), so files aren't replaced while it's running.
3. Syncs `prototype-code/app/` to `/home/arduino/ArduinoApps/SEC-StockCarRace-Parallel-1/` with `rsync --delete`, so files you deleted locally are deleted on the board too.

**The script does not restart the app.** Start it from Arduino App Lab, or on the board:

```bash
arduino-app-cli app start /home/arduino/ArduinoApps/SEC-StockCarRace-Parallel-1
```

You'll be asked for the board's password once per deploy. The script reuses a single SSH connection for the stop and sync steps.

### Finding the board's IP

The script reads the `UNO_IP` environment variable. If it's unset (or the board is no longer reachable at that address), the script prints these instructions and prompts you:

1. On the Uno Q (App Lab terminal, serial console, or `adb shell`), run `ip -brief addr`.
2. Use the address on the **`wlan0`** interface (or `eth0` if wired). Ignore `docker0`, `br-*`, and `usb0`; they won't work from your laptop.
3. Your laptop and the board must be on the same network.

Use the IP, not a `.local` name, because `.local` names usually don't resolve in WSL2.

After you enter it, the script prints an `export UNO_IP=...` line. Paste it into your terminal to skip the prompt for the rest of that session. Because our network assigns a new IP each time, you'll normally do this once per session.

### Optional overrides

| Variable     | Default                                                  | Purpose                 |
| ------------ | -------------------------------------------------------- | ----------------------- |
| `UNO_IP`     | _(prompted)_                                             | Board IP address        |
| `UNO_USER`   | `arduino`                                                | SSH user on the board   |
| `REMOTE_APP` | `/home/arduino/ArduinoApps/SEC-StockCarRace-Parallel-1/` | App folder on the board |

**Sharing a board?** Set a different `REMOTE_APP` for each person. Otherwise deploys overwrite each other, and `--delete` will remove a teammate's files.

## Pull changes back (Uno Q → laptop)

If you edited the app directly on the board (for example in App Lab) and want those changes in your repo:

```bash
./prototype-code/scripts/unoq-sync.sh pull
```

It first shows a dry run of what would change, then asks for confirmation before copying anything. Confirm within about a minute and you won't be asked for the password a second time. Pull never deletes local files. After pulling, review with `git status` and `git diff`, and commit what you want to keep.

To avoid conflicts, treat the repo on your laptop as the source of truth. Deploy to test, and only edit on the board for quick fixes that you pull and commit right away. The next `deploy` overwrites whatever is on the board.

## Deploying to the Uno Q (Windows)

First time only:
    `.\deploy setup-key`

Then:
- `.\deploy deploy` push your changes to the board
- `.\deploy pull` bring the board's changes back
- `.\deploy restart` also: start, stop

If the board's IP changes, the script asks for the new one.

## Notes

- `.git`, `__pycache__`, `.venv`, and `.cache` are never synced in either direction. `.cache` holds App Lab's build and library cache on the board, and keeping it makes restarts faster. Make sure `.cache/` is in `.gitignore`.
- If the app shows up twice in App Lab, check for a stray second folder with `ls ~/ArduinoApps` on the board, stop it, and delete it.

## Troubleshooting

| Symptom                                                  | Likely cause / fix                                                                                                                                                                                   |
| -------------------------------------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `Could not resolve hostname ...local`                    | Use the board's IP, not a `.local` name.                                                                                                                                                             |
| Script says the IP isn't reachable                       | Wrong interface (use `wlan0`), different network, or SSH isn't enabled on the board.                                                                                                                 |
| IP looks right but still unreachable on guest Wi-Fi      | Guest networks often block devices from talking to each other. Try a phone hotspot that both your laptop and the board join.                                                                         |
| SSH asks "Are you sure you want to continue connecting?" | Normal for an IP you haven't connected to before. To verify it's really your board, compare the fingerprint with `ssh-keygen -lf /etc/ssh/ssh_host_ed25519_key.pub` on the board, then answer `yes`. |
| `rsync: connection unexpectedly closed` or `bash: line 1: rsync: command not found` | `rsync` is missing on the Uno Q. In an App Lab terminal on the board, run `sudo apt update && sudo apt install -y rsync`, then retry. |
| `WARNING: REMOTE HOST IDENTIFICATION HAS CHANGED`        | The IP was reassigned to a different device (or your board). Confirm it's your board, then run `ssh-keygen -R <ip>`.                                                                                 |
| `Permission denied`                                      | Wrong password. Check the `arduino` user's password on the board.                                                                                                                                    |

## Repeat AM32 drive motors

Use one [Repeat AM32 drive ESC](https://repeat-robotics.com/products/repeat-am32-drive-esc) per
[Repeat Drive Pro motor](https://repeat-robotics.com/products/repeat-drive-pro). Connect each
motor's three phase wires to its own ESC. Wire the left ESC signal to UNO Q **D11** and the right
ESC signal to **D9**. Tie both ESC signal grounds to UNO Q GND. Power the ESCs from a battery
within both the ESC and the installed motor variant's ratings, and power the UNO Q separately.
The UNO Q signal is 3.3 V; keep battery voltage off its headers.

The sketch expects bidirectional **RC servo input** (AM32 Auto or Servo, with bidirectional drive
and normal immediate reverse). It sends pulses about every 20 ms using Arduino's `Servo` library
and defaults to 1000 / 1450 / 2000 µs for reverse / neutral / forward before direction inversion.
Leave the Repeat motor settings in place. If either ESC has been reconfigured, check its input
mode and servo endpoints/neutral in the [AM32 configurator](https://github.com/am32-firmware/AM32#firmware-release--configuration-tool)
before changing the pulse constants in `TeamOneMotorController.cpp`. Neutral is defined in
`sketch.ino`.

The App Lab sketch profile declares `Servo` 1.3.0, which supports the UNO Q. If building in
Arduino IDE, select **Arduino UNO Q**, install **Servo 1.3.0** in Library Manager, and open
`app/sketch/sketch.ino`.

1. Raise both wheels off the ground and start the sketch in its default `DriveMode::Idle`.
   Confirm both ESCs arm and neither wheel moves. If checking the signal with a scope, look
   for a 1450 µs high pulse about every 20 ms on both pins.
2. Change `DRIVE_MODE` in `sketch.ino` to `DriveMode::BenchTest` and restart with the ESCs
   powered. After three seconds at neutral, the sketch commands each wheel forward and reverse
   at about one quarter throttle for 500 ms each, with a neutral pause between commands. It
   then stays at neutral until restart. The test runs again on every restart in this mode.
3. If a wheel labeled forward spins backward, flip that side's `*_ESC_REVERSED` flag and
   repeat. Set `DRIVE_MODE` back to `Idle` after testing. `PrototypeCourse` opts into the old
   course routine below.

### Calibrate ESC pulses in the Serial Monitor

Set `DRIVE_MODE` to `DriveMode::PulseCalibration` and build once. Secure the car with
both wheels off theg ground. Start the app on the UNO Q first so it starts
sending 1450 µs to both ESCs. Then power on the robot and let the ESCs arm. You
can unplug the computer's USB cable once the UNO Q has its own power. `Ready`
means the Monitor accepts commands. The ESCs keep receiving neutral until you
send a valid wheel command. Once the ESCs have armed, send one command per line. Reconnect over Wi-Fi if you unplugged
 with 
`arduino-app-cli monitor`USB. For a movement check, send `l 1625` or `r 1625` and look for the `accepted`
message. The commands below let you check neutral or end a trial.

| Command | Action |
| --- | --- |
| `l 1495` | Send a raw 1495 µs pulse to the left ESC |
| `r 1505` | Send a raw 1505 µs pulse to the right ESC |
| `stop` | Return both ESCs to the controller's baseline 1450 µs pulse |

The range is 1000–2000 µs. Each trial lasts five seconds, then both ESCs return to
1450 µs. The other ESC stays at its configured neutral during the trial.
Keep the battery disconnect within reach. Raw pulses bypass the wheel reversal
flags. Start at 1450 µs and try a few microseconds on either side to find the
center of each wheel's observed no-motion range. Record the results separately.
Pulse trials are temporary. Select the desired drive mode before the competition.

### Test the wheel encoders under power

Select `DriveMode::EncoderTest`. Secure the car with both wheels raised. Start
the UNO Q first, power on the robot, and let the ESCs arm. Follow the prompts
in **Console → Serial Monitor**. Each reply tests the named wheel. The sketch
zerthe output monitormed wheel forward to one encoder revolution
(8192 counts), pauses for the tape check, then reverses to the starting count.
It prints both settle to check that it rotated exactly 360 degreesch move so you can see any overshoot.
Each move times out after 15 seconds and returns both ESCs to neutral. Restart
the app to repeat the test. The encoder channels are on D4/D5 (left) and D7/D6 (right).
The ultrasonic trigger and echo are on D2 and D3.

### Test IMU heading hold

With `DriveMode::HeadingTest` selected, start the app, power on the robot,
and let the ESCs arm. Set the car on a flat, clear straightaway and aim it
down the line. Send `run 8 0.25 0.2` in the Serial Monitor and leave the car
still while the gyro calibrates for about two seconds. The arguments are
duration in seconds, base throttle, and proportional heading gain. Use
`run 8 0.25 0` to compare with steering correction disabled. Both ESCs return
to neutral and the Monitor reports final and peak heading deviation.

`IMU_YAW_REVERSED` in `sketch.ino` assumes the sensor's +Z axis points up
when set to `false`. To check the sign, send `yaw`, hold the car still through
calibration, then turn it left by hand when prompted. Hold the new angle until
the test prints final yaw. The motors stay at neutral. If final yaw is negative
after a left turn, flip `IMU_YAW_REVERSED` and repeat the check.

### Compare encoder matching and gyro correction

With `DriveMode::StraightTest` selected, let the ESCs arm and aim the car
down a clear straightaway. Use the Serial Monitor to compare these runs
from the same starting position:

| Command | Test |
| --- | --- |
| `wheels 4 0.2 0.22 0` | Fixed throttle baseline for each wheel |
| `wheels 4 0.2 0.22 0.3` | Encoder correction that targets equal wheel revolutions |
| `both 4 0.2 0.22 0.3 1` | Same encoder correction with a gyro-adjusted travel target |

The fields are seconds, left base throttle, right base throttle, wheel gain,
and gyro gain for `both`. Adjust the base throttles until both wheels launch
reliably. The example gives the right side a small extra command to try first.
Keep still during gyro calibration in a combined trial. Each run starts the
encoder counts at zero and reports wheel travel after commanding neutral.

The same mode offers two separate RPM-target tests:

| Command | Test |
| --- | --- |
| `rpm 4 30 0.2 0.22 0.001 0` | Both wheels track 30 RPM independently with P control |
| `rpm 4 30 0.2 0.22 0.001 0.0002` | Same targets with a small integral term |
| `rpmimu 4 30 0.2 0.22 0.001 0.0002 20` | Same wheel loops with gyro steering |

The fields are seconds, target encoder-shaft RPM, left and right starting
throttle, speed Kp and Ki, and gyro gain for `rpmimu`. The gyro gain is in
RPM per radian. Gyro correction can shift each target by up to 25% of the
base RPM. Start with a target both wheels can reach and adjust the starting
throttles if one wheel launches late. After each run, compare average RPM,
peak RPM difference, and, with the gyro enabled, peak yaw deviation.

### Run the prototype course

Adjust course distances, steering gains, and pivot settings at the top of
`PrototypeCourse.cpp`. Wheel diameter and hardware settings are in `sketch.ino`.

Select `DriveMode::PrototypeCourse`. After the ESCs arm, line up the robot at the start
of the right-side straight, facing towards the track. Send `start` in the
Serial Monitor and keep the car still while the gyro calibrates. Send `stop`
to interrupt a run. After stopping, realign the car at the starting point
before sending `start` again.

The straights use the same controller as `both`, with base throttles
0.20 and 0.25, wheel gain 0.25, and gyro gain 0.35. Average encoder travel
ends each straight. Each 90° left pivot slows near its gyro target, then
checks the settled angle and makes short corrections if needed. The next
straight starts within 5° of target. The course repeats four
straight-and-turn segments per lap and stops after three laps.

Send `straight` to check the first leg and stop before the pivot. The Monitor
reports estimated inches and average wheel revolutions at the turn-in point.
The one-time launch from the start square is 62 inches. After the first left
pivot, the upper loop alternates 18-inch cross-track and 42-inch long legs.
The distance calculation uses the measured 67 mm wheel diameter.

### Track-test handoff

One of eight logged attempts completed three laps using the 62 / 18 / 42-inch
course settings. Keep those distances as the baseline while diagnosing the
failures below. The log has no lap times, so the 40-second target is unverified.

- **Runaway pivots (two attempts).** The robot spun rapidly during the failed
  pivots. The Monitor reported settled heading errors of 149.91° and 174.59°.
  These are wrapped errors relative to the target heading. The turn code
  recomputes the commanded direction from that error on every update, even
  during a correction pulse. Isolate one pivot and log yaw, the target, and
  both throttle commands through the correction pulses. Check for direction
  changes and stop the pivot on a large overshoot before retrying full laps.
- **IMU read failures (two attempts).** The operator noticed these around
  turns. A single failed read stops the course. Check that the IMU and its
  connections are secure, then see whether reads fail during a powered pivot.
  Loose wiring and motor-related electrical interference are possible causes.
- **Straight-line wobble and wall contact (three heading cutoffs).** The
  operator saw the car weave into a wall on several runs. The logged cutoffs
  were all near −20°, after 10.10 inches on the first short leg, 55.86 inches
  on the entry, and 7.56 inches on the first long leg. The cutoff measures
  heading and can trigger after wall contact. Retest `straight` at lower
  throttle and watch lateral clearance as well as heading.

Start by checking the IMU wiring, then isolate a pivot and retest a straight
before running the full course again.
