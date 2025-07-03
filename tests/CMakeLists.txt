cmake_minimum_required(VERSION 3.31)
project(YeetMouseTests)

set(CMAKE_CXX_STANDARD 17)

if (CMAKE_SYSTEM_PROCESSOR STREQUAL "ppc64le")
    message(STATUS "Configuring for ppc64le architecture. Adding __ppc64le__ definition.")
    add_compile_definitions(__ppc64le__)
endif()

file(GLOB driver_source "../driver/accel_modes.[c|h]")
file(GLOB fixedpoint_source "../driver/FixedMath/*")

# Copy driver source files over
file(COPY ${driver_source} DESTINATION "${CMAKE_HOME_DIRECTORY}/driver")
file(COPY ${fixedpoint_source} DESTINATION "${CMAKE_HOME_DIRECTORY}/driver/FixedMath")
file(COPY "../shared_definitions.h" DESTINATION "${CMAKE_HOME_DIRECTORY}")

# Run a python script to clean-up the freshly copied source files
execute_process(COMMAND python3 CleanUpFiles.py
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})

add_executable(YeetMouseTests main.cpp driver/accel_modes.c
        driver/config.h
        TestManager.cpp
        TestManager.h
        Tests.cpp
        Tests.h
        ../gui/FunctionHelper.cpp)
