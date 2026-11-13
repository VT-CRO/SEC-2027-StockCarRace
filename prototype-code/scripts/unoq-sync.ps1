param([Parameter(Position = 0)][string]$Command = "")
$ErrorActionPreference = "Stop"

# Locate the app relative to this script
$RepoRoot  = (git -C $PSScriptRoot rev-parse --show-toplevel).Trim()
$LocalApp  = Join-Path $RepoRoot "prototype-code\app"
$UnoUser   = if ($env:UNO_USER)   { $env:UNO_USER }   else { "arduino" }
$RemoteApp = if ($env:REMOTE_APP) { $env:REMOTE_APP } else { "/home/arduino/ArduinoApps/SEC-StockCarRace-Parallel-1/" }
$AppName   = "user:SEC-StockCarRace-Parallel-1"
$AppPath   = $RemoteApp.TrimEnd("/")
$Excludes  = @(".git", "__pycache__", ".venv", ".cache")

function Test-IPv4($s) {
    $ip = $null
    return ($s -match '^(\d{1,3}\.){3}\d{1,3}$') -and [System.Net.IPAddress]::TryParse($s, [ref]$ip)
}

function Test-Reachable($ip) {
    $client = New-Object System.Net.Sockets.TcpClient
    try {
        $task = $client.ConnectAsync($ip, 22)
        return ($task.Wait(5000) -and $client.Connected)
    } catch {
        return $false
    } finally {
        $client.Dispose()
    }
}

function Read-UnoIp {
    Write-Host @"

How to find the Uno Q's IP address:
  1. On the Uno Q (App Lab terminal, serial, or adb), run:
         ip -brief addr
  2. Use the address on the wlan0 interface (or eth0 if wired).
     Usually looks like 192.168.x.x or 10.x.x.x.
     Ignore docker0 / br-* (172.x.x.x) and usb0, they won't work from here.
  3. Your laptop and the board must be on the same network.

"@
    while ($true) {
        $ip = Read-Host "Uno Q IP address"
        if (Test-IPv4 $ip) { break }
        Write-Host "That doesn't look like an IPv4 address (e.g. 192.168.1.42). Try again."
    }
    if (Test-Reachable $ip) {
        Write-Host "OK: $ip is reachable on port 22."
    } else {
        Write-Host "Warning: could not reach $ip on port 22 within 5s."
        Write-Host "  - Wrong interface? Re-check 'ip -brief addr' on the board."
        Write-Host "  - Guest Wi-Fi often isolates devices. Try a phone hotspot."
        $ans = Read-Host "Use it anyway? [y/N]"
        if ($ans -notmatch '^[Yy]$') { exit 1 }
    }
    $env:UNO_IP = $ip
    Write-Host ""
    Write-Host "To skip this prompt for the rest of this terminal session, run:"
    Write-Host ""
    Write-Host "    `$env:UNO_IP = `"$ip`""
    Write-Host ""
}

# Resolve the board IP: environment first, otherwise prompt
if (-not $env:UNO_IP) {
    Write-Host "UNO_IP is not set."
    Read-UnoIp
} elseif (-not (Test-IPv4 $env:UNO_IP) -or -not (Test-Reachable $env:UNO_IP)) {
    Write-Host "UNO_IP='$($env:UNO_IP)' is invalid or not reachable (the IP may have changed)."
    Read-UnoIp
}

$UnoHost = "$UnoUser@$($env:UNO_IP)"

function Invoke-Remote($cmd, [switch]$IgnoreFailure) {
    ssh $UnoHost $cmd
    if ($LASTEXITCODE -ne 0 -and -not $IgnoreFailure) {
        throw "Remote command failed: $cmd"
    }
}

function Assert-Ok($what) {
    if ($LASTEXITCODE -ne 0) { throw "$what failed." }
}

switch ($Command) {
    "deploy" {
        Invoke-Remote "arduino-app-cli app stop '$AppName'" -IgnoreFailure

        $tmp = Join-Path $env:TEMP "unoq-app.tar"
        $tarArgs = @("-cf", $tmp) + ($Excludes | ForEach-Object { "--exclude=$_" }) + @("-C", $LocalApp, ".")
        tar @tarArgs
        Assert-Ok "Packing the app"

        scp $tmp "$($UnoHost):/tmp/unoq-app.tar"
        Assert-Ok "Copying to the board"
        Remove-Item $tmp

        # Clear the old files (keeping excluded folders), then unpack the new ones
        $keep = ($Excludes | ForEach-Object { "! -name '$_'" }) -join " "
        Invoke-Remote "mkdir -p '$AppPath' && find '$AppPath' -mindepth 1 -maxdepth 1 $keep -exec rm -rf {} + && tar -xf /tmp/unoq-app.tar -C '$AppPath' && rm /tmp/unoq-app.tar"
        Write-Host "Deployed."
    }
    "pull" {
        $remoteEx = ($Excludes | ForEach-Object { "--exclude=$_" }) -join " "
        Invoke-Remote "tar -cf /tmp/unoq-pull.tar $remoteEx -C '$AppPath' ."

        $tmpTar = Join-Path $env:TEMP "unoq-pull.tar"
        $tmpDir = Join-Path $env:TEMP "unoq-pull"
        if (Test-Path $tmpDir) { Remove-Item $tmpDir -Recurse -Force }
        New-Item -ItemType Directory $tmpDir | Out-Null

        scp "$($UnoHost):/tmp/unoq-pull.tar" $tmpTar
        Assert-Ok "Copying from the board"
        Invoke-Remote "rm /tmp/unoq-pull.tar" -IgnoreFailure
        tar -xf $tmpTar -C $tmpDir
        Assert-Ok "Unpacking"

        # Show what would change locally
        $changes = @()
        Get-ChildItem $tmpDir -Recurse -File | ForEach-Object {
            $rel   = $_.FullName.Substring($tmpDir.Length).TrimStart("\")
            $local = Join-Path $LocalApp $rel
            if (-not (Test-Path $local)) {
                $changes += [pscustomobject]@{ Status = "new";     Path = $rel }
            } elseif ((Get-FileHash $_.FullName).Hash -ne (Get-FileHash $local).Hash) {
                $changes += [pscustomobject]@{ Status = "changed"; Path = $rel }
            }
        }

        if (-not $changes) {
            Write-Host "No differences."
        } else {
            $changes | Format-Table -AutoSize
            $ans = Read-Host "Apply these changes? [y/N]"
            if ($ans -match '^[Yy]$') {
                Copy-Item "$tmpDir\*" $LocalApp -Recurse -Force
                Write-Host "Applied."
            }
        }
        Remove-Item $tmpTar, $tmpDir -Recurse -Force
    }
    "stop"    { Invoke-Remote "arduino-app-cli app stop '$AppPath'" -IgnoreFailure }
    "setup-key" {
        $keyPath = Join-Path $env:USERPROFILE ".ssh\id_ed25519"
        if (-not (Test-Path "$keyPath.pub")) {
            New-Item -ItemType Directory (Split-Path $keyPath) -Force | Out-Null
            Write-Host "Creating an SSH key. Press Enter at each question."
            ssh-keygen -t ed25519 -f $keyPath
            Assert-Ok "Creating the key"
        }
        Write-Host "Installing the key on the board. Type the board password one last time."
        Get-Content "$keyPath.pub" | ssh $UnoHost 'mkdir -p ~/.ssh && chmod 700 ~/.ssh && tr -d "\r" >> ~/.ssh/authorized_keys && chmod 600 ~/.ssh/authorized_keys'
        Assert-Ok "Installing the key"
        Write-Host "Done. Deploys should no longer ask for a password."
    }
    "start"   {
        Invoke-Remote "arduino-app-cli app stop '$AppPath'" -IgnoreFailure
        Invoke-Remote "arduino-app-cli app start '$AppPath'" -IgnoreFailure
    }
    "restart" { Invoke-Remote "arduino-app-cli app restart '$AppPath'" -IgnoreFailure }
    default {
        Write-Host "Usage: .\deploy.cmd {deploy|pull|stop|start|restart|setup-key}"
        Write-Host "Board: $UnoHost  (override with UNO_IP / UNO_USER / REMOTE_APP)"
        exit 1
    }
}