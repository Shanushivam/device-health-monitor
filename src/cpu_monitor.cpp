#include "cpu_monitor.h"
#include <fstream>
#include <sstream>
#include <thread>
#include <chrono>

namespace {
bool read_cpu(unsigned long long& idle, unsigned long long& total) {
    std::ifstream file("/proc/stat");
    std::string cpu;
    unsigned long long user, nice, system, idle_v, iowait, irq, softirq, steal;
    if (!(file >> cpu >> user >> nice >> system >> idle_v >> iowait >> irq >> softirq >> steal))
        return false;
    idle = idle_v + iowait;
    total = user + nice + system + idle_v + iowait + irq + softirq + steal;
    return true;
}
}

double get_cpu_usage() {
    unsigned long long idle1, total1, idle2, total2;
    if (!read_cpu(idle1, total1)) return -1.0;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    if (!read_cpu(idle2, total2)) return -1.0;

    const auto total_delta = total2 - total1;
    const auto idle_delta = idle2 - idle1;
    if (total_delta == 0) return 0.0;
    return 100.0 * (1.0 - static_cast<double>(idle_delta) / total_delta);
}
