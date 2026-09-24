#include "service_monitor.h"
#include <algorithm>
#include <cstdlib>
#include <string>

bool is_service_active(const std::string& service_name) {
    const std::string command = "systemctl is-active --quiet " + service_name;
    return std::system(command.c_str()) == 0;
}

bool restart_service(const std::string& service_name) {
    const std::string command = "systemctl restart " + service_name;
    return std::system(command.c_str()) == 0;
}

int restart_backoff_seconds(int consecutive_failures, int interval_seconds) {
    const long long cap = std::max(300, interval_seconds);
    long long delay = interval_seconds;
    for (int i = 0; i < consecutive_failures && delay < cap; ++i) delay *= 2;
    return static_cast<int>(std::min(delay, cap));
}
