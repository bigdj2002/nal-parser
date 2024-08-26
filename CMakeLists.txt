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

# Set installation directories
set(CMAKE_INSTALL_PREFIX ${PROJECT_SOURCE_DIR}/install)
set(INSTALL_BIN_DIR ${CMAKE_INSTALL_PREFIX}/bin)
set(INSTALL_INCLUDE_DIR ${CMAKE_INSTALL_PREFIX}/include)
set(INSTALL_LIB_DIR ${CMAKE_INSTALL_PREFIX}/lib)

# Specify installation rules
install(TARGETS nalparser
        RUNTIME DESTINATION ${INSTALL_BIN_DIR} # For executable files
        LIBRARY DESTINATION ${INSTALL_LIB_DIR} # For shared libraries
        ARCHIVE DESTINATION ${INSTALL_LIB_DIR} # For static libraries
)

install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/ DESTINATION ${INSTALL_INCLUDE_DIR})

# Add the tests
add_subdirectory(tests)
