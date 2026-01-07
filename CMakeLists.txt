cmake_minimum_required(VERSION 3.10)

project(ArbSim)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler warnings
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -pedantic)
endif()

# Optional: Enable per-event timing (adds overhead, useful for profiling)
option(ENABLE_PER_EVENT_TIMING "Enable per-event timing instrumentation" OFF)
if(ENABLE_PER_EVENT_TIMING)
    add_compile_definitions(ENABLE_PER_EVENT_TIMING)
endif()

# Include directories
include_directories(src)

# Core library sources (shared between main app and tests)
set(CORE_SOURCES
    src/config/Config.cpp
    src/core/CsvReader.cpp
    src/core/PnlTracker.cpp
    src/core/SimulationEngine.cpp
    src/core/Strategy.cpp
    src/core/StreamMerger.cpp
)

# Main executable
add_executable(ArbSim src/app/Main.cpp ${CORE_SOURCES})

# Test executable
add_executable(ArbSimTests Tests/BasicTests.cpp ${CORE_SOURCES})

# Enable testing
enable_testing()
add_test(NAME BasicTests COMMAND ArbSimTests)

# Copy config and data to build directory
file(COPY config/config.cfg DESTINATION ${CMAKE_BINARY_DIR})
file(COPY data DESTINATION ${CMAKE_BINARY_DIR})
