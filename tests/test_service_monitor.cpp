#undef NDEBUG
#include "service_monitor.h"
#include <cassert>
#include <iostream>

int main() {
    // systemd may not be available in every CI/container environment.
    // Verify that the function can be called without crashing.
    const bool active = is_service_active("nonexistent-health-monitor-test-service");
    assert(!active);

    // Backoff doubles per consecutive failure and is capped at five minutes.
    assert(restart_backoff_seconds(1, 5) == 10);
    assert(restart_backoff_seconds(2, 5) == 20);
    assert(restart_backoff_seconds(6, 5) == 300);
    assert(restart_backoff_seconds(1000, 5) == 300);
    // An interval longer than the cap is never shortened.
    assert(restart_backoff_seconds(3, 600) == 600);
    assert(restart_backoff_seconds(1, 86400) == 86400);

    std::cout << "Service monitor test passed.\n";
}
