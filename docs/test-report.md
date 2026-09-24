# Test Report

## Test environment

Recommended environment:

- Linux distribution: Ubuntu/Debian
- Compiler: GCC with C++17 support
- CMake: 3.16+
- nlohmann-json development package
- systemd for service-recovery testing

## Automated tests

| Test | Purpose | Expected result |
|---|---|---|
| `test_cpu` | Read `/proc/stat` and calculate CPU percentage | 0–100% |
| `test_memory` | Read `/proc/meminfo` | 0–100% |
| `test_config` | Parse JSON configuration | Valid defaults/values |
| `test_service_monitor` | Check inactive/nonexistent service | Returns false |

Run:

```bash
cd build
ctest --output-on-failure
```

## Manual health tests

### CPU

Run a CPU load generator and observe the CPU warning threshold.

### RAM

Consume memory temporarily and verify that the configured threshold produces a warning.

### Disk

Create a temporary large file only if sufficient free space exists, then verify disk monitoring.

### Temperature

Run a sustained workload and observe thermal-zone readings where the hardware exposes them.

### Network

Disconnect the configured interface or temporarily block connectivity. The monitor should log network failure.

### Critical service

```bash
sudo systemctl stop ssh
```

The monitor should detect the failure and attempt:

```bash
systemctl restart ssh
```

## Expected recovery log

```text
[CRITICAL] Critical service is inactive: ssh
[WARNING] Attempting service restart (attempt 1)...
[INFO] Service restarted successfully.
```

## Limitations

Temperature sensor names and network interface names vary across Linux devices. Some containers or virtual machines do not expose `/sys/class/thermal` or systemd. The implementation therefore logs unavailable sensors rather than treating them as a compile/runtime failure.
