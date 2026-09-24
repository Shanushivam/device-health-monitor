# Embedded Linux Device Health Monitor & Auto-Recovery Agent

A lightweight C++17 daemon for Linux devices (Raspberry Pi, industrial boards, servers) that
continuously checks device health, logs problems, and automatically restarts a critical
systemd service when it goes down.

## Features

- **Six health checks** every few seconds: CPU, RAM, disk, temperature, network and one critical service
- **Auto-recovery**: restarts the critical service through `systemctl` and confirms it is running again
- **Restart backoff**: after a failed restart, waits twice as long before each new attempt (up to 5 minutes), so a broken service is not restarted in a tight loop
- **Validated JSON configuration**: invalid or unsafe values stop the monitor at startup with a clear error
- **Clean shutdown**: stops within a fraction of a second on `SIGINT` / `SIGTERM`
- **systemd integration**: install scripts and a unit file for running as a background service
- **No heavy dependencies**: only the C++ standard library and [nlohmann/json](https://github.com/nlohmann/json)

## How it works

| Check | Data source | Reaction |
|---|---|---|
| CPU usage | `/proc/stat` (two samples, 100 ms apart) | `WARNING` above threshold |
| RAM usage | `/proc/meminfo` (`MemTotal` − `MemAvailable`) | `WARNING` above threshold |
| Disk usage | `statvfs("/")` | `WARNING` above threshold |
| Temperature | Hottest zone in `/sys/class/thermal/thermal_zone*` | `CRITICAL` above threshold |
| Network | `/sys/class/net/<interface>/operstate` + one ping | `ERROR` when down or unreachable |
| Critical service | `systemctl is-active` | `CRITICAL`, then restart with backoff |

A reading that cannot be taken (for example, no thermal sensor) is logged as a warning rather
than stopping the monitor. See [docs/architecture.md](docs/architecture.md) for details.

## Requirements

- Linux with `/proc` and `/sys` (Ubuntu, Debian, Raspberry Pi OS, …)
- systemd, for service monitoring and recovery
- GCC or Clang with C++17 support, CMake 3.16+, nlohmann-json

On Ubuntu/Debian:

```bash
sudo apt update
sudo apt install -y build-essential cmake nlohmann-json3-dev
```

## Build and test

```bash
mkdir -p build && cd build
cmake ..
cmake --build .
ctest --output-on-failure
```

The tests are small standalone programs using standard C++ assertions, so no test framework is required.

## Run

From the project root:

```bash
sudo ./build/device-health-monitor
```

Without an argument, the monitor uses the first configuration file it finds:

1. `config/health_monitor.json`
2. `../config/health_monitor.json`
3. `/etc/device-health-monitor/health_monitor.json`

To use a specific file:

```bash
sudo ./build/device-health-monitor path/to/config.json
```

To run without `sudo`, set `logging.file` to a writable path. Restarting a service still needs root.

Example output:

```text
[2026-09-24 19:15:20] [INFO] Device Health Monitor started with config: config/health_monitor.json
[2026-09-24 19:15:20] [INFO] CPU usage: 12.5%
[2026-09-24 19:15:20] [INFO] RAM usage: 41.3%
[2026-09-24 19:15:20] [INFO] Disk usage: 58.0129%
[2026-09-24 19:15:20] [INFO] System temperature: 48.2 C
[2026-09-24 19:15:20] [INFO] Network healthy.
[2026-09-24 19:15:20] [CRITICAL] Critical service is inactive: ssh
[2026-09-24 19:15:20] [WARNING] Attempting service restart (attempt 1)...
[2026-09-24 19:15:21] [INFO] Service restarted successfully.
```

## Configuration

All settings live in [config/health_monitor.json](config/health_monitor.json):

```json
{
    "interval_seconds": 5,
    "thresholds": {
        "cpu_percent": 80.0,
        "memory_percent": 75.0,
        "disk_percent": 80.0,
        "temperature_celsius": 70.0
    },
    "network": {
        "interface": "eth0",
        "ping_target": "8.8.8.8"
    },
    "critical_service": {
        "name": "ssh",
        "restart_on_failure": true
    },
    "logging": {
        "file": "/var/log/device-health-monitor.log"
    }
}
```

| Setting | Meaning | Allowed values |
|---|---|---|
| `interval_seconds` | Time between checks | Whole number, 1–86400 |
| `thresholds.*_percent` | Usage level that triggers a warning | Above 0, at most 100 |
| `thresholds.temperature_celsius` | Temperature that triggers a critical alert | Above 0, at most 150 |
| `network.interface` | Interface to check, e.g. `eth0`, `wlan0` | Letters, digits, `. _ : -`; max 15 characters |
| `network.ping_target` | Host or IP address to ping | Letters, digits, `. : -` |
| `critical_service.name` | systemd service to watch, e.g. `ssh`, `sshd`, `nginx` | Letters, digits, `@ . _ : -` |
| `critical_service.restart_on_failure` | Restart the service automatically | `true` / `false` |
| `logging.file` | Log file (messages also go to the console) | Any writable path |

Any missing setting falls back to the default shown above. Names may not start with `-`.
This validation keeps shell commands safe, because these values are passed to `ping` and `systemctl`.

## Install as a systemd service

```bash
sudo ./scripts/install.sh
```

This installs the binary to `/usr/local/bin`, the configuration to
`/etc/device-health-monitor/`, and enables the `device-health-monitor` service.
Reinstalling overwrites the configuration in `/etc`.

```bash
systemctl status device-health-monitor
journalctl -u device-health-monitor -f
```

To remove it:

```bash
sudo ./scripts/uninstall.sh
```

## Try the auto-recovery

With the monitor running, stop the watched service:

```bash
sudo ./scripts/simulate_failure.sh ssh
```

Within one check interval the monitor logs the failure and restarts the service.

## Project structure

```text
├── config/health_monitor.json      # Default configuration
├── include/                        # Headers, one per module
├── src/
│   ├── main.cpp                    # Main loop, alerts and recovery logic
│   ├── config_manager.cpp          # JSON loading and validation
│   ├── cpu_monitor.cpp             # /proc/stat
│   ├── memory_monitor.cpp          # /proc/meminfo
│   ├── disk_monitor.cpp            # statvfs
│   ├── temperature_monitor.cpp     # /sys/class/thermal
│   ├── network_monitor.cpp         # operstate + ping
│   ├── service_monitor.cpp         # systemctl + restart backoff
│   └── logger.cpp                  # Console and file logging
├── tests/                          # Assertion-based test programs
├── scripts/                        # install, uninstall, simulate_failure
├── systemd/                        # Unit file
└── docs/                           # Architecture and test report
```

## Known limitations

- Linux only; the CPU, RAM and temperature tests need `/proc` and `/sys`.
- Every reading is logged on every check. Use log rotation (e.g. `logrotate`) on long-running devices.
- The service runs as root, and the systemd unit has no sandboxing options yet.
- Only one critical service can be watched.

## Roadmap

- Log only changes of state (healthy → unhealthy and back) to reduce log volume
- Watch several services
- Harden the systemd unit (`NoNewPrivileges`, `ProtectSystem`, …)
- Run commands without a shell (`fork`/`execvp`)

## Credits

Originally created by **[Sampreeti Mohapatra](https://github.com/sampreeti-mohapatra/embedded-linux-health-monitor)**.

Extended and maintained by **[Shanushivam](https://github.com/Shanushivam)**, including:

- Fixed the Clang/macOS build error in the logger
- Config file lookup that works from the project root, the build directory or `/etc`
- Input validation to prevent shell command injection through the configuration
- Range and type checks for the interval and thresholds, with clear error messages
- Fast, clean shutdown on `SIGINT` / `SIGTERM`
- Restart backoff and a check that the service is actually running after a restart
- A warning when the log file cannot be written
- More robust `/proc/meminfo` parsing, and use of the hottest thermal zone
- Warnings when CPU, RAM or disk readings are unavailable
- Stricter default thresholds and additional tests
