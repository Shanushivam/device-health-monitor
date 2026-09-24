#pragma once
#include <optional>

// Hottest thermal zone in degrees Celsius, or no value when no sensor is readable.
std::optional<double> get_system_temperature();
