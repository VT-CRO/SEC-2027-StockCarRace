#!/usr/bin/env bash
# deploy.sh — push an Arduino App from WSL to an UNO Q over SSH, then (re)start it.
#
# Usage:
#   ./deploy.sh [APP_NAME] [--no-logs] [--no-monitor] [--keep-running]
#
#   After starting, it opens the serial monitor (sketch Monitor.print output)
#   with the app logs streaming alongside.
#   Ctrl+C (or closing the terminal) STOPS the app on the board,
#   unless --keep-running is given.
#
# Lives in src/arduino-apps/ next to the apps it deploys:
#   src/arduino-apps/
#   ├── deploy.sh
#   └── prototype-code/        <- APP_NAME
#       ├── app.yaml
#       ├── python/main.py
#       └── sketch/sketch.ino
#
# APP_NAME is optional when there's only one app in this folder.
# Can be run from anywhere (e.g. the workspace root):
#   src/arduino-apps/deploy.sh prototype-code
#
# Config — precedence: command-line env var > .env file > default
#   UNOQ_HOST   ssh target (an alias from ~/.ssh/config, or arduino@192.168.x.x)
#   REMOTE_ROOT where apps live on the board
#   ENV_FILE    path to the .env file (default: workspace root, two levels up)
#   PASSWORD    optional board login password (needs `sudo apt install sshpass`);
#               leave it out if you've set up SSH keys with ssh-copy-id

set -euo pipefail

# Resolve everything relative to this script, not the caller's cwd
APPS_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# --- load .env from the workspace root ---------------------------------------
ENV_FILE="${ENV_FILE:-$APPS_ROOT/../../.env}"
if [[ -f "$ENV_FILE" ]]; then
  # remember anything set on the command line so .env doesn't override it
  _cli_host="${UNOQ_HOST-}"
  _cli_root="${REMOTE_ROOT-}"
  set -a                                         # export everything the file defines
  source <(sed 's/\r$//' "$ENV_FILE")            # strip CRLF in case it was edited in Windows
  set +a
  [[ -n "$_cli_host" ]] && UNOQ_HOST="$_cli_host"
  [[ -n "$_cli_root" ]] && REMOTE_ROOT="$_cli_root"
fi

UNOQ_HOST="${UNOQ_HOST:-unoq}"
REMOTE_ROOT="${REMOTE_ROOT:-/home/arduino/ArduinoApps}"

# --- ssh setup ---------------------------------------------------------------
# One shared connection for the whole run (ControlMaster): you authenticate once,
# every later ssh/rsync call reuses it instead of asking again.
mkdir -p "$HOME/.ssh"
SSH_OPTS=(-o ControlMaster=auto -o "ControlPath=$HOME/.ssh/cm-%C" -o ControlPersist=60)

# Optional: PASSWORD in .env → log in non-interactively via sshpass.
# (SSH keys are the better fix; see ssh-copy-id.)
SSH_PREFIX=()
if [[ -n "${PASSWORD:-}" ]]; then
  command -v sshpass >/dev/null || { echo "✗ PASSWORD is set but sshpass isn't installed. Run: sudo apt install sshpass"; exit 1; }
  export SSHPASS="$PASSWORD"   # sshpass -e reads it from the env, so it never shows up in `ps`
  unset PASSWORD
  SSH_PREFIX=(sshpass -e)
fi

rssh() { "${SSH_PREFIX[@]}" ssh "${SSH_OPTS[@]}" "$@"; }
RSYNC_SSH="${SSH_PREFIX[*]} ssh ${SSH_OPTS[*]}"   # same thing as a string, for rsync -e

APP_NAME=""
FOLLOW_LOGS=1
OPEN_MONITOR=1
KEEP_RUNNING=0
for arg in "$@"; do
  case "$arg" in
    --no-logs) FOLLOW_LOGS=0 ;;
    --no-monitor) OPEN_MONITOR=0 ;;
    --keep-running) KEEP_RUNNING=1 ;;
    *) APP_NAME="$(basename "$arg")" ;;   # accepts "prototype-code" or "prototype-code/"
  esac
done

# Every subfolder with an app.yaml counts as an app
mapfile -t APPS < <(find "$APPS_ROOT" -mindepth 2 -maxdepth 2 -name app.yaml -printf '%h\n' | xargs -rn1 basename | sort)

if [[ -z "$APP_NAME" ]]; then
  if [[ ${#APPS[@]} -eq 1 ]]; then
    APP_NAME="${APPS[0]}"
  else
    echo "Usage: $0 <app-name> [--no-logs] [--no-monitor] [--keep-running]"
    echo "Apps found: ${APPS[*]:-none}"
    exit 1
  fi
fi

APP_DIR="$APPS_ROOT/$APP_NAME"
REMOTE_APP="$REMOTE_ROOT/$APP_NAME"

# --- sanity checks -----------------------------------------------------------
[[ -f "$APP_DIR/app.yaml" ]] || { echo "✗ No app '$APP_NAME' in $APPS_ROOT (apps found: ${APPS[*]:-none})"; exit 1; }

if [[ "$APP_DIR" == /mnt/* ]]; then
  echo "⚠ App is on the Windows drive ($APP_DIR). Works, but slower and can mangle permissions."
fi

# CRLF line endings break Python/shell on the board's Debian side
if grep -rIl $'\r' "$APP_DIR" --include='*.py' --include='*.yaml' --include='*.sh' 2>/dev/null | grep -q .; then
  echo "⚠ CRLF line endings found. Fix with: find '$APP_DIR' -name '*.py' -exec sed -i 's/\r$//' {} +"
fi

echo "→ Checking connection to $UNOQ_HOST..."
rssh -o ConnectTimeout=5 "$UNOQ_HOST" true || { echo "✗ Can't reach or log in to $UNOQ_HOST (wrong PASSWORD?)"; exit 1; }
trap 'ssh "${SSH_OPTS[@]}" -O exit "$UNOQ_HOST" 2>/dev/null || true' EXIT   # close the shared connection when done

# --- Ctrl+C / terminal closed → stop the app on the board -------------------
APP_STARTED=0   # flipped to 1 right before `app start`
LOGS_PID=""
on_interrupt() {
  trap '' INT TERM HUP                                 # don't let a 2nd Ctrl+C cut the cleanup short
  set +e                                               # cleanup must run to the end even if a step fails
  [[ -n "$LOGS_PID" ]] && kill "$LOGS_PID" 2>/dev/null # (usually already dead from the same Ctrl+C)
  echo
  if [[ $APP_STARTED -eq 1 && $KEEP_RUNNING -eq 0 ]]; then
    echo "→ Interrupted — stopping $APP_NAME on the board..."
    if rssh -n -o ConnectTimeout=5 "$UNOQ_HOST" "arduino-app-cli app stop '$REMOTE_APP'" >/dev/null 2>&1; then
      echo "✓ $APP_NAME stopped"
    else
      echo "⚠ Couldn't stop it. On the board run: arduino-app-cli app stop user:$APP_NAME"
    fi
  elif [[ $APP_STARTED -eq 1 ]]; then
    echo "→ Detached (--keep-running): $APP_NAME is still running on the board"
  else
    echo "→ Interrupted before the app was started; nothing to stop"
  fi
  exit 130
}
trap on_interrupt INT TERM HUP

# --- sync files --------------------------------------------------------------
echo "→ Syncing $APP_NAME → $UNOQ_HOST:$REMOTE_APP"
rssh "$UNOQ_HOST" "mkdir -p '$REMOTE_APP'"

# rsync has to exist on BOTH ends
if command -v rsync >/dev/null && rssh "$UNOQ_HOST" command -v rsync >/dev/null 2>&1; then
  rsync -az --delete -e "$RSYNC_SSH" \
    --exclude '.git/' --exclude '.cache/' --exclude '__pycache__/' \
    --exclude '.venv/' --exclude '*.pyc' --exclude '.vscode/' \
    "$APP_DIR/" "$UNOQ_HOST:$REMOTE_APP/"
else
  echo "  (rsync missing locally or on the board — falling back to tar; 'sudo apt install rsync' on both to speed this up)"
  tar -C "$APP_DIR" --exclude='.git' --exclude='.cache' --exclude='__pycache__' --exclude='.venv' -czf - . \
    | rssh "$UNOQ_HOST" "rm -rf '$REMOTE_APP'/* && tar -C '$REMOTE_APP' -xzf -"
fi

# --- stop, then start (start compiles + flashes the sketch and launches python) ---
# The microcontroller is shared, so stop EVERY running app, not just this one.
# `app ps` prints a header, then one row per app:  ID  STATUS  NAME
echo "→ Stopping running apps..."
RUNNING="$(rssh "$UNOQ_HOST" "arduino-app-cli app ps" 2>/dev/null | awk 'NR>1 && NF {print $1}')" || true
for id in $RUNNING; do
  echo "  stopping $id"
  rssh "$UNOQ_HOST" "arduino-app-cli app stop '$id'" >/dev/null 2>&1 || true
done
[[ -z "$RUNNING" ]] && echo "  (none running)"
# also stop this app by path, in case it's half-running and not listed
rssh "$UNOQ_HOST" "arduino-app-cli app stop '$REMOTE_APP'" >/dev/null 2>&1 || true

# Logs keep history across runs and `app logs` replays the last --tail lines
# (default 100). Count what's there now, so we can skip it and show only this run.
LOG_TAIL=1000000   # big enough to always fetch the full history
OLD_LOG_LINES="$(rssh -n "$UNOQ_HOST" "arduino-app-cli app logs '$REMOTE_APP' --tail $LOG_TAIL" 2>/dev/null | wc -l)" || OLD_LOG_LINES=0
# sed -u = unbuffered, so each log line shows up the moment it arrives
# (awk/mawk and tail both hold piped input back until the stream ends)
new_logs_only() { sed -u -n "$((OLD_LOG_LINES + 1)),\$p"; }

echo "→ Compiling + starting $APP_NAME..."
# Don't let set -e kill the script here: on failure we still want the logs.
START_RC=0
APP_STARTED=1   # from here on, Ctrl+C should stop it (stopping a half-started app is harmless)
# No -t: with a terminal attached, closing the session (or Ctrl+C) sends a signal
# to the remote side, which can tear the app down. App Lab never does that.
rssh "$UNOQ_HOST" "arduino-app-cli app start '$REMOTE_APP'" || START_RC=$?

if [[ $START_RC -ne 0 ]]; then
  echo
  echo "✗ app start failed (exit $START_RC). App logs:"
  echo "────────────────────────────────────────────"
  rssh -n "$UNOQ_HOST" "arduino-app-cli app logs '$REMOTE_APP' --tail $LOG_TAIL" 2>&1 | new_logs_only | tail -n 100 || true
  exit "$START_RC"
fi

echo "✓ $APP_NAME started"

# --- logs + monitor ----------------------------------------------------------
# Logs catch runtime errors like Python tracebacks; --follow keeps streaming new lines.
# Monitor shows the sketch's Monitor.print() output (Serial.print goes to the UART pins instead).
# No -t on either, so Ctrl+C only closes the local viewers and never reaches the board.
LOGS_CMD="arduino-app-cli app logs '$REMOTE_APP' --follow --tail $LOG_TAIL"   # piped through new_logs_only
if [[ $KEEP_RUNNING -eq 1 ]]; then STOP_HINT="app keeps running"; else STOP_HINT="this also STOPS the app"; fi
MONITOR_CMD="arduino-app-cli monitor"

if [[ $OPEN_MONITOR -eq 1 ]]; then
  if [[ $FOLLOW_LOGS -eq 1 ]]; then
    # -n: the background logs must not read stdin, or they'd steal what you type into the monitor
    rssh -n "$UNOQ_HOST" "$LOGS_CMD" | new_logs_only &
    LOGS_PID=$!
  fi
  echo "→ Opening serial monitor (logs stream alongside; type + Enter to send; Ctrl+C to close — $STOP_HINT)"
  rssh "$UNOQ_HOST" "$MONITOR_CMD" || true
  [[ -n "$LOGS_PID" ]] && kill "$LOGS_PID" 2>/dev/null || true   # monitor closed on its own → stop logs too
elif [[ $FOLLOW_LOGS -eq 1 ]]; then
  echo "→ Streaming logs (Ctrl+C to close — $STOP_HINT)"
  rssh -n "$UNOQ_HOST" "$LOGS_CMD" | new_logs_only || true
fi