cmake_minimum_required(VERSION 3.28)
project(driver)

set(CMAKE_CXX_STANDARD 26)

add_library(my_pam_fp SHARED pam_fp.cpp
        serial/serial.cpp
        serial/packet.cpp serial/packet.h
        serial/converters.h
)

add_executable(sandbox
        sandbox.cpp
        serial/serial.cpp
        serial/packet.cpp serial/packet.h
        serial/converters.h
)

add_executable(pam_fp_test
        pam_fp_test.cpp
        serial/serial.cpp
        serial/packet.cpp serial/packet.h
        serial/converters.h
)

target_link_libraries(sandbox PUBLIC my_pam_fp pam pam_misc)
target_link_libraries(pam_fp_test PUBLIC my_pam_fp pam pam_misc)

