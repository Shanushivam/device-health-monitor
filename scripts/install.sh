#!/bin/bash
set -e

PROJECT_DIR="$(cd "$(dirname "$0")/.." && pwd)"

if [ "$EUID" -ne 0 ]; then
    echo "Please run as root: sudo $0"
    exit 1
fi

if [ ! -f "$PROJECT_DIR/build/device-health-monitor" ]; then
    echo "Build the project first:"
    echo "  mkdir -p build && cd build && cmake .. && cmake --build ."
    exit 1
fi

mkdir -p /etc/device-health-monitor
cp "$PROJECT_DIR/config/health_monitor.json" /etc/device-health-monitor/health_monitor.json
cp "$PROJECT_DIR/build/device-health-monitor" /usr/local/bin/device-health-monitor
cp "$PROJECT_DIR/systemd/device-health-monitor.service" /etc/systemd/system/device-health-monitor.service

chmod 755 /usr/local/bin/device-health-monitor
chmod 644 /etc/device-health-monitor/health_monitor.json

systemctl daemon-reload
systemctl enable --now device-health-monitor

echo "Device Health Monitor installed and started."
