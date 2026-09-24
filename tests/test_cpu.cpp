#undef NDEBUG
#include "cpu_monitor.h"
#include <cassert>
#include <iostream>

int main() {
    const double value = get_cpu_usage();
    assert(value >= 0.0 && value <= 100.0);
    std::cout << "CPU test passed: " << value << "%\n";
}
