#undef NDEBUG
#include "memory_monitor.h"
#include <cassert>
#include <iostream>

int main() {
    const double value = get_memory_usage();
    assert(value >= 0.0 && value <= 100.0);
    std::cout << "Memory test passed: " << value << "%\n";
}
