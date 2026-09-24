#pragma once
#include <string>

struct Thresholds {
    double cpu_percent = 80.0;
    double memory_percent = 75.0;
    double disk_percent = 80.0;
    double temperature_celsius = 70.0;
};

struct NetworkConfig {
    std::string interface_name = "eth0";
    std::string ping_target = "8.8.8.8";
};

struct ServiceConfig {
    std::string name = "ssh";
    bool restart_on_failure = true;
};

struct Config {
    int interval_seconds = 5;
    Thresholds thresholds;
    NetworkConfig network;
    ServiceConfig service;
    std::string log_file = "/var/log/device-health-monitor.log";
};

Config load_config(const std::string& path);
