#include "network_monitor.h"
#include <fstream>
#include <cstdlib>

bool is_interface_up(const std::string& interface_name) {
    std::ifstream file("/sys/class/net/" + interface_name + "/operstate");
    std::string state;
    return static_cast<bool>(file >> state) && (state == "up" || state == "unknown");
}

bool can_reach_host(const std::string& host) {
    const std::string command = "ping -c 1 -W 2 " + host + " > /dev/null 2>&1";
    return std::system(command.c_str()) == 0;
}
