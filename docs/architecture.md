# Architecture

## Overview

The application is a long-running Linux process. `main.cpp` coordinates independent monitoring modules.

```text
                         +----------------------+
                         |       main.cpp       |
                         |  health supervisor   |
                         +----------+-----------+
                                    |
        +---------------------------+---------------------------+
        |            |              |             |             |
        v            v              v             v             v
      CPU          Memory          Disk      Temperature     Network
        |            |              |             |             |
        +------------+--------------+-------------+-------------+
                                    |
                                    v
                           Service Monitor
                                    |
                                    v
                              Logger / Action
```

## Data sources

### CPU

Reads `/proc/stat` twice with a short interval and calculates the percentage of non-idle CPU time.

### RAM

Reads `/proc/meminfo` and calculates:

`used = MemTotal - MemAvailable`

### Disk

Uses `statvfs()` on `/` to calculate filesystem usage.

### Temperature

Reads every Linux thermal zone under `/sys/class/thermal/thermal_zone*` and reports the hottest one.

### Network

Checks the configured network interface under `/sys/class/net/<interface>/operstate`, then performs one ICMP ping to the configured target.

### Critical service

Uses `systemctl is-active` to determine whether the configured systemd service is running. If it is inactive and recovery is enabled, the monitor runs `systemctl restart` and confirms the service is active afterwards. After a failed attempt, the next attempt is delayed: the check interval is doubled per consecutive failure, capped at five minutes.

## Configuration

All thresholds and the critical service are externalized in JSON so the binary does not need to be rebuilt when operational thresholds change.

## Process lifecycle

The daemon handles `SIGINT` and `SIGTERM`, allowing a clean shutdown within a fraction of a second, even during the wait between checks. systemd also restarts the process if the monitor itself exits unexpectedly.

## Security considerations

The service runs as root because service restart and system-wide logging can require privileges. A production version should minimize privileges where possible and validate any command arguments carefully.
