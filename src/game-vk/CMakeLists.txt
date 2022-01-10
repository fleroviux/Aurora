cmake_minimum_required(VERSION 3.2)
project(Aurora-Game-Vk CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/CMakeModules)

set(SOURCES
  src/main.cpp
)

set(HEADERS
)

include(FindSDL2)
find_package(SDL2 REQUIRED)
find_package(Vulkan REQUIRED)

add_executable(Aurora-Game-Vk ${SOURCES} ${HEADERS})
target_link_libraries(Aurora-Game-Vk Aurora-Renderer PRIVATE ${SDL2_LIBRARY} Vulkan::Vulkan)
target_include_directories(Aurora-Game-Vk PRIVATE ${SDL2_INCLUDE_DIR})