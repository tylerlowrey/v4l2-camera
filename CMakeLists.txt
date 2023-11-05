cmake_minimum_required(VERSION 3.26)
project(video4linux2_rpi C)

set(CMAKE_C_STANDARD 11)

add_executable(video4linux2_rpi main.c
        camera.h
        camera.c)
