#include "disk_monitor.h"
#include <sys/statvfs.h>

double get_disk_usage(const char* path) {
    struct statvfs fs {};
    if (statvfs(path, &fs) != 0 || fs.f_blocks == 0) return -1.0;

    const unsigned long long total = fs.f_blocks;
    const unsigned long long available = fs.f_bavail;
    return 100.0 * (1.0 - static_cast<double>(available) / total);
}
