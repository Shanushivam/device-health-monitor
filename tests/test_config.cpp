#undef NDEBUG
#include "config_manager.h"
#include <cassert>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
bool loads(const std::string& json) {
    const std::string path = "test_config_tmp.json";
    std::ofstream(path) << json;
    try {
        load_config(path);
        std::remove(path.c_str());
        return true;
    } catch (const std::runtime_error&) {
        std::remove(path.c_str());
        return false;
    }
}
}

int main() {
    const Config c = load_config(PROJECT_SOURCE_DIR "/config/health_monitor.json");
    assert(c.interval_seconds > 0);
    assert(c.thresholds.cpu_percent > 0);
    assert(!c.service.name.empty());

    // Defaults apply when sections are missing.
    assert(loads("{}"));
    assert(loads(R"({"critical_service": {"name": "getty@tty1.service"}})"));

    // Values that reach the shell or /sys paths are rejected.
    assert(!loads(R"({"critical_service": {"name": "ssh; reboot"}})"));
    assert(!loads(R"({"critical_service": {"name": "--help"}})"));
    assert(!loads(R"({"network": {"ping_target": "8.8.8.8 && id"}})"));
    assert(!loads(R"({"network": {"interface": "../../etc"}})"));

    // Numeric ranges and section types are enforced.
    assert(loads(R"({"interval_seconds": 1})"));
    assert(loads(R"({"interval_seconds": 86400})"));
    assert(!loads(R"({"interval_seconds": 0})"));
    assert(!loads(R"({"interval_seconds": -5})"));
    assert(!loads(R"({"interval_seconds": 2.7})"));
    assert(!loads(R"({"interval_seconds": 99999999999})"));
    assert(!loads(R"({"thresholds": {"cpu_percent": -10}})"));
    assert(!loads(R"({"thresholds": {"memory_percent": 120}})"));
    assert(!loads(R"({"thresholds": {"temperature_celsius": 0}})"));
    assert(!loads(R"({"thresholds": null})"));
    assert(!loads(R"([1, 2, 3])"));

    std::cout << "Config test passed.\n";
}
