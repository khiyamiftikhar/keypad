# This is the project CMakeLists.txt file for the test subproject
cmake_minimum_required(VERSION 3.16)

# Include the components directory of the main application:
#
set(EXTRA_COMPONENT_DIRS "../components"  "$ENV{IDF_PATH}/tools/unit-test-app/components")

# Set the components to include the tests for.
# This can be overriden from CMake cache:
# - when invoking CMake directly: cmake -D TEST_COMPONENTS="xxxxx" ..
# - when using idf.py: idf.py -T xxxxx build
#
#set(TEST_COMPONENTS "05HAL" CACHE STRING "List of components to test")
set(TEST_COMPONENTS "timer_impl")
set(CONFIG_UNIT_TESTING 0)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(unit_test_test)