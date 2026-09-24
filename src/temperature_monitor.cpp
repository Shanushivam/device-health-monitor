#include "temperature_monitor.h"
#include <filesystem>
#include <fstream>
#include <string>

std::optional<double> get_system_temperature() {
    namespace fs = std::filesystem;
    const std::string base = "/sys/class/thermal";

    std::optional<double> hottest;

    std::error_code ec;
    for (const auto& entry : fs::directory_iterator(base, ec)) {
        if (entry.path().filename().string().rfind("thermal_zone", 0) != 0)
            continue;

        std::ifstream temp_file(entry.path() / "temp");
        long long milli = 0;
        if (!(temp_file >> milli)) continue;

        const double celsius = static_cast<double>(milli) / 1000.0;
        if (!hottest || celsius > *hottest) hottest = celsius;
    }

    return hottest;
}
