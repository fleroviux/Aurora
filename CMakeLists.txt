cmake_minimum_required(VERSION 3.2)
project(Aurora)

add_subdirectory(external)
add_subdirectory(src/common)
add_subdirectory(src/game ${CMAKE_CURRENT_BINARY_DIR}/bin/game/)
add_subdirectory(src/math)
add_subdirectory(src/scene)