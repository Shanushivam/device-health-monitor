#include "cpu_monitor.h"
#include "memory_monitor.h"
#include "disk_monitor.h"
#include "temperature_monitor.h"
#include "network_monitor.h"
#include "service_monitor.h"
#include "logger.h"
#include "config_manager.h"

#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <csignal>
#include <sstream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <string>

namespace {
std::atomic<bool> running{true};

void handle_signal(int) {
    running = false;
}

std::string format_value(double value, const char* unit) {
    std::ostringstream out;
    out << value << unit;
    return out.str();
}

// Without an explicit argument, use the project config relative to either the
// project root or the build directory, then the installed config.
std::string find_config_path() {
    const std::vector<std::string> candidates = {
        "config/health_monitor.json",
        "../config/health_monitor.json",
        "/etc/device-health-monitor/health_monitor.json",
    };
    for (const auto& path : candidates) {
        if (std::ifstream(path)) return path;
    }
    return candidates.front();
}

// Sleeps in short slices so SIGINT/SIGTERM stop the monitor promptly.
void sleep_while_running(std::chrono::seconds duration) {
    const auto deadline = std::chrono::steady_clock::now() + duration;
    while (running && std::chrono::steady_clock::now() < deadline) {
        const auto remaining = deadline - std::chrono::steady_clock::now();
        std::this_thread::sleep_for(std::min<std::chrono::steady_clock::duration>(
            remaining, std::chrono::milliseconds(200)));
    }
}
}

int main(int argc, char* argv[]) {
    const std::string config_path = argc > 1 ? argv[1] : find_config_path();

    try {
        Config config = load_config(config_path);
        Logger logger(config.log_file);

        std::signal(SIGINT, handle_signal);
        std::signal(SIGTERM, handle_signal);

        logger.log(LogLevel::INFO, "Device Health Monitor started with config: " + config_path);

        int restart_failures = 0;
        auto next_restart_at = std::chrono::steady_clock::time_point{};

        while (running) {
            const double cpu = get_cpu_usage();
            const double memory = get_memory_usage();
            const double disk = get_disk_usage("/");
            const auto temperature = get_system_temperature();
            const bool interface_up = is_interface_up(config.network.interface_name);
            const bool network_ok = interface_up && can_reach_host(config.network.ping_target);
            const bool service_ok = is_service_active(config.service.name);

            if (cpu >= 0) {
                logger.log(cpu > config.thresholds.cpu_percent ? LogLevel::WARNING : LogLevel::INFO,
                           "CPU usage: " + format_value(cpu, "%"));
            } else {
                logger.log(LogLevel::WARNING, "CPU usage unavailable.");
            }

            if (memory >= 0) {
                logger.log(memory > config.thresholds.memory_percent ? LogLevel::WARNING : LogLevel::INFO,
                           "RAM usage: " + format_value(memory, "%"));
            } else {
                logger.log(LogLevel::WARNING, "RAM usage unavailable.");
            }

            if (disk >= 0) {
                logger.log(disk > config.thresholds.disk_percent ? LogLevel::WARNING : LogLevel::INFO,
                           "Disk usage: " + format_value(disk, "%"));
            } else {
                logger.log(LogLevel::WARNING, "Disk usage unavailable.");
            }

            if (temperature) {
                logger.log(*temperature > config.thresholds.temperature_celsius ? LogLevel::CRITICAL : LogLevel::INFO,
                           "System temperature: " + format_value(*temperature, " C"));
            } else {
                logger.log(LogLevel::WARNING, "Temperature sensor unavailable.");
            }

            if (!network_ok) {
                logger.log(LogLevel::ERROR, "Network unhealthy: interface=" +
                           config.network.interface_name + ", target=" + config.network.ping_target);
            } else {
                logger.log(LogLevel::INFO, "Network healthy.");
            }

            if (!service_ok) {
                logger.log(LogLevel::CRITICAL, "Critical service is inactive: " + config.service.name);
                const auto now = std::chrono::steady_clock::now();
                if (config.service.restart_on_failure && now >= next_restart_at) {
                    logger.log(LogLevel::WARNING, "Attempting service restart (attempt " +
                               std::to_string(restart_failures + 1) + ")...");
                    // Confirm the service is actually running, not just that systemctl returned.
                    if (restart_service(config.service.name) && is_service_active(config.service.name)) {
                        logger.log(LogLevel::INFO, "Service restarted successfully.");
                        restart_failures = 0;
                    } else {
                        ++restart_failures;
                        const int delay = restart_backoff_seconds(restart_failures, config.interval_seconds);
                        next_restart_at = now + std::chrono::seconds(delay);
                        logger.log(LogLevel::CRITICAL, "Service restart failed. Next attempt in " +
                                   std::to_string(delay) + "s.");
                    }
                } else if (config.service.restart_on_failure) {
                    const auto wait = std::chrono::ceil<std::chrono::seconds>(next_restart_at - now);
                    logger.log(LogLevel::WARNING, "Restart postponed; next attempt in " +
                               std::to_string(wait.count()) + "s.");
                }
            } else {
                logger.log(LogLevel::INFO, "Critical service healthy: " + config.service.name);
                restart_failures = 0;
                next_restart_at = {};
            }

            sleep_while_running(std::chrono::seconds(config.interval_seconds));
        }

        logger.log(LogLevel::INFO, "Device Health Monitor stopped.");
    } catch (const std::exception& ex) {
        std::cerr << "Fatal error: " << ex.what() << '\n';
        return 1;
    }

    return 0;
}
