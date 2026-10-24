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
2. **Uno Q:** complete the normal first-time setup in Arduino App Lab, connect the board to Wi-Fi, and make sure SSH is enabled.
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
| `WARNING: REMOTE HOST IDENTIFICATION HAS CHANGED`        | The IP was reassigned to a different device (or your board). Confirm it's your board, then run `ssh-keygen -R <ip>`.                                                                                 |
| `Permission denied`                                      | Wrong password. Check the `arduino` user's password on the board.                                                                                                                                    |
