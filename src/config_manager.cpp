#include "config_manager.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <stdexcept>

using json = nlohmann::json;

namespace {
constexpr int max_interval_seconds = 24 * 60 * 60;

// Values below are passed to shell commands or used in /sys paths, so only a
// conservative character set is accepted. A leading '-' would be parsed as an option.
bool is_safe_token(const std::string& value, const std::string& extra_chars) {
    if (value.empty() || value.size() > 255 || value[0] == '-') return false;
    for (const char ch : value) {
        const bool alnum = (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9');
        if (!alnum && extra_chars.find(ch) == std::string::npos) return false;
    }
    return true;
}

// Returns the named section, or nullptr when it is absent. A present section must be an object.
const json* section(const json& j, const char* name) {
    if (!j.contains(name)) return nullptr;
    const json& s = j.at(name);
    if (!s.is_object()) throw std::runtime_error(std::string("\"") + name + "\" must be a JSON object");
    return &s;
}

double read_threshold(const json& t, const char* key, double fallback, double max) {
    const double value = t.value(key, fallback);
    if (!(value > 0.0 && value <= max))
        throw std::runtime_error(std::string("thresholds.") + key + " must be greater than 0 and at most " +
                                 std::to_string(static_cast<int>(max)));
    return value;
}

void validate(const Config& c) {
    if (!is_safe_token(c.service.name, "@._:-"))
        throw std::runtime_error("Invalid critical_service.name: " + c.service.name);
    if (!is_safe_token(c.network.ping_target, ".:-"))
        throw std::runtime_error("Invalid network.ping_target: " + c.network.ping_target);
    if (!is_safe_token(c.network.interface_name, "._:-") || c.network.interface_name.size() > 15 ||
        c.network.interface_name == "." || c.network.interface_name == "..")
        throw std::runtime_error("Invalid network.interface: " + c.network.interface_name);
    if (c.log_file.empty())
        throw std::runtime_error("logging.file must not be empty");
}
}

Config load_config(const std::string& path) {
    std::ifstream file(path);
    if (!file) throw std::runtime_error("Cannot open configuration: " + path);

    json j;
    file >> j;
    if (!j.is_object()) throw std::runtime_error("Configuration must be a JSON object: " + path);

    Config c;

    if (j.contains("interval_seconds")) {
        const json& interval = j.at("interval_seconds");
        // Check the range on the JSON number itself; converting first would overflow int.
        const bool in_range = (interval.is_number_unsigned() && interval.get<unsigned long long>() >= 1 &&
                               interval.get<unsigned long long>() <= max_interval_seconds) ||
                              (interval.is_number_integer() && interval.get<long long>() >= 1 &&
                               interval.get<long long>() <= max_interval_seconds);
        if (!in_range)
            throw std::runtime_error("interval_seconds must be a whole number from 1 to " +
                                     std::to_string(max_interval_seconds));
        c.interval_seconds = interval.get<int>();
    }

    if (const json* t = section(j, "thresholds")) {
        c.thresholds.cpu_percent = read_threshold(*t, "cpu_percent", c.thresholds.cpu_percent, 100.0);
        c.thresholds.memory_percent = read_threshold(*t, "memory_percent", c.thresholds.memory_percent, 100.0);
        c.thresholds.disk_percent = read_threshold(*t, "disk_percent", c.thresholds.disk_percent, 100.0);
        c.thresholds.temperature_celsius =
            read_threshold(*t, "temperature_celsius", c.thresholds.temperature_celsius, 150.0);
    }

    if (const json* n = section(j, "network")) {
        c.network.interface_name = n->value("interface", c.network.interface_name);
        c.network.ping_target = n->value("ping_target", c.network.ping_target);
    }

    if (const json* s = section(j, "critical_service")) {
        c.service.name = s->value("name", c.service.name);
        c.service.restart_on_failure = s->value("restart_on_failure", c.service.restart_on_failure);
    }

    if (const json* l = section(j, "logging"))
        c.log_file = l->value("file", c.log_file);

    validate(c);
    return c;
}
