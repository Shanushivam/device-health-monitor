#!/bin/bash
set -e

SERVICE="${1:-ssh}"

if [ "$EUID" -ne 0 ]; then
    echo "Run as root: sudo $0 [service]"
    exit 1
fi

echo "Stopping $SERVICE to simulate a service failure..."
systemctl stop "$SERVICE"
echo "The health monitor should detect the failure and attempt recovery."
