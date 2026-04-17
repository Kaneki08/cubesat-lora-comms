cmake_minimum_required(VERSION 3.28)
project(cubesat_lora_comms)

set(CMAKE_CXX_STANDARD 14)

include_directories(src)

add_executable(cubesat_lora_comms
        src/HopeRFTX.cpp
        src/HopeRFTX.h)
