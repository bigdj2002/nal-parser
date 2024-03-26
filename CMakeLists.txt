cmake_minimum_required(VERSION 3.10)
project(NalParser)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Include directories
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/include/h264)
include_directories(${PROJECT_SOURCE_DIR}/include/hevc)
include_directories(${PROJECT_SOURCE_DIR}/include/vvc)
include_directories(${PROJECT_SOURCE_DIR}/common)
include_directories(${PROJECT_SOURCE_DIR}/api)

# Source files
file(GLOB_RECURSE SOURCES
    ${PROJECT_SOURCE_DIR}/src/*.cpp
)

# Add the main library target
add_library(nalparser ${SOURCES})

# Add the tests
add_subdirectory(tests)
