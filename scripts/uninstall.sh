#!/bin/bash
set -e

if [ "$EUID" -ne 0 ]; then
    echo "Please run as root: sudo $0"
    exit 1
fi

systemctl disable --now device-health-monitor 2>/dev/null || true
rm -f /etc/systemd/system/device-health-monitor.service
rm -f /usr/local/bin/device-health-monitor
rm -rf /etc/device-health-monitor

systemctl daemon-reload

echo "Device Health Monitor uninstalled."
