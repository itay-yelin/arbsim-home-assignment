cmake_minimum_required(VERSION 3.10)

project(ArbSim)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories
# "src" allows #include "core/File.h" and "config/File.h"
# "src/core" allows internal includes if needed, though sibling includes usually work.
include_directories(src)

# Source files
set(SOURCES
    src/app/Main.cpp
    src/config/Config.cpp
    src/core/CsvReader.cpp
    src/core/PnlTracker.cpp
    src/core/SimulationEngine.cpp
    src/core/Strategy.cpp
    src/core/StreamMerger.cpp
)

add_executable(ArbSim ${SOURCES})

# Ensure config.cfg is copied to build output for easy running
# or the user runs from project root. We'll copy to build dir just in case.
file(COPY config/config.cfg DESTINATION ${CMAKE_BINARY_DIR})
file(COPY data DESTINATION ${CMAKE_BINARY_DIR})
