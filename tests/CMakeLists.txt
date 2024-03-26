# Add test executable
add_executable(test_parse test_parse.cpp)

# Link test executable with the main library
target_link_libraries(test_parse nalparser)
