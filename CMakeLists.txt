cmake_minimum_required(VERSION 3.26)

project(video4linux2_rpi C)

set(CMAKE_C_STANDARD 11)

include_directories(${CMAKE_SOURCE_DIR}/include)
link_directories(${CMAKE_SOURCE_DIR}/lib)

add_executable(video4linux2_rpi main.c
        camera.h
        camera.c
        helper.h
        helper.c)
target_link_libraries(video4linux2_rpi apriltag)
