cmake_minimum_required(VERSION 3.26)

project(video4linux2_rpi C)

set(CMAKE_C_STANDARD 11)

include_directories(${CMAKE_SOURCE_DIR}/include src)
link_directories(${CMAKE_SOURCE_DIR}/lib)
aux_source_directory(src SRC_LIST)

add_executable(video4linux2_rpi ${SRC_LIST})
target_link_libraries(video4linux2_rpi apriltag)
