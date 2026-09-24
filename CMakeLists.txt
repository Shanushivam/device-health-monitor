cmake_minimum_required(VERSION 3.16)
project(embedded_linux_health_monitor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(nlohmann_json REQUIRED)

add_executable(device-health-monitor
    src/main.cpp
    src/cpu_monitor.cpp
    src/memory_monitor.cpp
    src/disk_monitor.cpp
    src/temperature_monitor.cpp
    src/network_monitor.cpp
    src/service_monitor.cpp
    src/logger.cpp
    src/config_manager.cpp
)

target_include_directories(device-health-monitor PRIVATE include)
target_link_libraries(device-health-monitor PRIVATE nlohmann_json::nlohmann_json)

enable_testing()

function(add_monitor_test test_name source_file)
    add_executable(${test_name} ${source_file}
        src/cpu_monitor.cpp
        src/memory_monitor.cpp
        src/disk_monitor.cpp
        src/temperature_monitor.cpp
        src/network_monitor.cpp
        src/service_monitor.cpp
        src/logger.cpp
        src/config_manager.cpp
    )
    target_include_directories(${test_name} PRIVATE include)
    target_link_libraries(${test_name} PRIVATE nlohmann_json::nlohmann_json)
    target_compile_definitions(${test_name} PRIVATE PROJECT_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
    add_test(NAME ${test_name} COMMAND ${test_name})
endfunction()

add_monitor_test(test_cpu tests/test_cpu.cpp)
add_monitor_test(test_memory tests/test_memory.cpp)
add_monitor_test(test_config tests/test_config.cpp)
add_monitor_test(test_service_monitor tests/test_service_monitor.cpp)
