#include "memory_monitor.h"
#include <fstream>
#include <sstream>
#include <string>

double get_memory_usage() {
    std::ifstream file("/proc/meminfo");
    std::string line;
    unsigned long long total = 0, available = 0;

    // Parse line by line: some entries (e.g. HugePages_Total) have no unit.
    while (std::getline(file, line)) {
        std::istringstream in(line);
        std::string key;
        unsigned long long value = 0;
        if (!(in >> key >> value)) continue;

        if (key == "MemTotal:") total = value;
        else if (key == "MemAvailable:") available = value;
    }

    if (total == 0 || available > total) return -1.0;
    return 100.0 * (1.0 - static_cast<double>(available) / total);
}
