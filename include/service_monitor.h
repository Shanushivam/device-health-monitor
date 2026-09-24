#pragma once
#include <string>
bool is_service_active(const std::string& service_name);
bool restart_service(const std::string& service_name);

// Seconds to wait before the next restart attempt after consecutive failed attempts:
// the check interval doubled per failure, capped at five minutes (or the interval, if longer).
int restart_backoff_seconds(int consecutive_failures, int interval_seconds);
