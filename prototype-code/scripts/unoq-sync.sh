#!/usr/bin/env bash
set -euo pipefail

# --- Locate the app relative to this script, wherever the repo is cloned ---
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(git -C "$SCRIPT_DIR" rev-parse --show-toplevel)"
LOCAL_APP="$REPO_ROOT/prototype-code/app/"

UNO_USER="${UNO_USER:-arduino}"
REMOTE_APP="${REMOTE_APP:-/home/arduino/ArduinoApps/SEC-StockCarRace-Parallel-1/}"

is_ipv4() {
  [[ "$1" =~ ^([0-9]{1,3}\.){3}[0-9]{1,3}$ ]] || return 1
  local IFS=. octet
  for octet in $1; do (( octet <= 255 )) || return 1; done
}

can_reach() {  # TCP check on the SSH port, 5s timeout
  timeout 5 bash -c "exec 3<>/dev/tcp/$1/22" 2>/dev/null
}

print_instructions() {
  cat <<'MSG'

How to find the Uno Q's IP address:
  1. On the Uno Q (App Lab terminal, serial, or adb), run:
         ip -brief addr
  2. Use the address on the wlan0 interface (or eth0 if wired).
     Usually looks like 192.168.x.x or 10.x.x.x.
     Ignore docker0 / br-* (172.x.x.x) and usb0 -- they won't work from WSL.
  3. Your laptop and the board must be on the same network.

(".local" names usually don't resolve in WSL2, so use the IP.)

MSG
}

prompt_for_ip() {
  if [[ ! -t 0 ]]; then
    echo "No valid UNO_IP and no terminal to prompt on." >&2
    echo "Run:  export UNO_IP=<board-ip>" >&2
    exit 1
  fi

  print_instructions

  local ip
  while true; do
    read -rp "Uno Q IP address: " ip
    if is_ipv4 "$ip"; then break; fi
    echo "That doesn't look like an IPv4 address (e.g. 192.168.1.42). Try again."
  done

  if can_reach "$ip"; then
    echo "OK: $ip is reachable on port 22."
  else
    echo "Warning: could not reach $ip on port 22 within 5s."
    echo "  - Wrong interface? Re-check 'ip -brief addr' on the board."
    echo "  - Guest Wi-Fi often isolates devices from each other. If the IP is"
    echo "    right and this still fails, try a phone hotspot."
    read -rp "Use it anyway? [y/N] " ans
    [[ "$ans" =~ ^[Yy]$ ]] || exit 1
  fi

  UNO_IP="$ip"
  echo
  echo "To skip this prompt for the rest of this terminal session, run:"
  echo
  echo "    export UNO_IP=$UNO_IP"
  echo
}

# --- Resolve the board IP: environment first, otherwise prompt ---
if [[ -z "${UNO_IP:-}" ]]; then
  echo "UNO_IP is not set."
  prompt_for_ip
elif ! is_ipv4 "$UNO_IP" || ! can_reach "$UNO_IP"; then
  echo "UNO_IP='$UNO_IP' is invalid or not reachable (the IP may have changed)."
  prompt_for_ip
fi
export UNO_IP

UNO_HOST="$UNO_USER@$UNO_IP"
EXCLUDES=(--exclude '.git' --exclude '__pycache__' --exclude '.venv' --exclude '.cache')
SSH_OPTS=(-o ControlMaster=auto -o ControlPath=/tmp/unoq-ssh-%C -o ControlPersist=60)

case "${1:-}" in
  deploy)
    APP_PATH="${REMOTE_APP%/}"
    ssh "${SSH_OPTS[@]}" "$UNO_HOST" "arduino-app-cli app stop '$APP_PATH'" || true
    rsync -avz --delete "${EXCLUDES[@]}" -e "ssh ${SSH_OPTS[*]}" "$LOCAL_APP" "$UNO_HOST:$REMOTE_APP"
    ;;
  pull)
    rsync -avzn "${EXCLUDES[@]}" -e "ssh ${SSH_OPTS[*]}" "$UNO_HOST:$REMOTE_APP" "$LOCAL_APP"
    read -rp "Apply these changes? [y/N] " ans
    [[ "$ans" =~ ^[Yy]$ ]] && rsync -avz "${EXCLUDES[@]}" -e "ssh ${SSH_OPTS[*]}" "$UNO_HOST:$REMOTE_APP" "$LOCAL_APP"
    ;;
  *)
    echo "Usage: $0 {deploy|pull}"
    echo "Board: $UNO_HOST  (override with UNO_IP / UNO_USER / REMOTE_APP)"
    exit 1
    ;;
esac