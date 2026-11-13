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
motor's three phase wires to its own ESC. Wire the left ESC signal to UNO Q **D9** and the right
ESC signal to **D10**. Tie both ESC signal grounds to UNO Q GND. Power the ESCs from a battery
within both the ESC and the installed motor variant's ratings, and power the UNO Q separately.
The UNO Q signal is 3.3 V; keep battery voltage off its headers.

The sketch expects bidirectional **RC servo input** (AM32 Auto or Servo, with bidirectional drive
and normal immediate reverse). It sends pulses about every 20 ms using Arduino's `Servo` library
and defaults to 1000 / 1500 / 2000 µs for reverse / neutral / forward before direction inversion.
Leave the Repeat motor settings in place. If either ESC has been reconfigured, check its input
mode and servo endpoints/neutral in the [AM32 configurator](https://github.com/am32-firmware/AM32#firmware-release--configuration-tool)
before changing the pulse constants in `TeamOneMotorController.cpp`.

The App Lab sketch profile declares `Servo` 1.3.0, which supports the UNO Q. If building in
Arduino IDE, select **Arduino UNO Q**, install **Servo 1.3.0** in Library Manager, and open
`app/sketch/sketch.ino`.

1. Raise both wheels off the ground and start the sketch in its default `DriveMode::Idle`.
   Confirm both ESCs arm and neither wheel moves. If checking the signal with a scope, look
   for a 1500 µs high pulse about every 20 ms on both pins.
2. Change `DRIVE_MODE` in `sketch.ino` to `DriveMode::BenchTest` and restart with the ESCs
   powered. After three seconds at neutral, the sketch commands each wheel forward and reverse
   at about one quarter throttle for 500 ms each, with a neutral pause between commands. It
   then stays at neutral until restart. The test runs again on every restart in this mode.
3. If a wheel labeled forward spins backward, flip that side's `*_ESC_REVERSED` flag and
   repeat. Set `DRIVE_MODE` back to `Idle` after testing. `PrototypeCourse` opts into the old
   timed driving and ultrasonic avoidance routine; its timings need to be tuned for the new
   chassis. The controller commands throttle and has no wheel speed or distance feedback.

### Calibrate ESC pulses in the Serial Monitor

Set `DRIVE_MODE` to `DriveMode::PulseCalibration` and build once. Secure the car with
both wheels clear of the bench. Start the app on the UNO Q first so it starts
sending 1500 µs to both ESCs. Then power on the robot and let the ESCs arm. You
can unplug the computer's USB cable once the UNO Q has its own power. `Ready`
means the Monitor accepts commands. The ESCs keep receiving neutral until you
send a valid wheel command. Once the ESCs have armed, send one command per line
in App Lab's **Console → Serial Monitor**. Reconnect over Wi-Fi if you unplugged
USB. For a movement check, send `l 1625` or `r 1625` and look for the `accepted`
message. The commands below let you check neutral or end a trial.

| Command | Action |
| --- | --- |
| `l 1495` | Send a raw 1495 µs pulse to the left ESC |
| `r 1505` | Send a raw 1505 µs pulse to the right ESC |
| `stop` | Return both ESCs to the controller's baseline 1500 µs pulse |

The range is 1000–2000 µs. Each trial lasts five seconds, then both ESCs return to
1500 µs. The other ESC stays at 1500 µs during the trial, so it may still twitch.
Keep the battery disconnect within reach. Raw pulses bypass the wheel reversal
flags. Start at 1500 µs and try a few microseconds on either side to find the
center of each wheel's observed no-motion range. Record the results separately.
Pulse trials are temporary. Select the desired drive mode before the competition.
