cmake_minimum_required(VERSION 3.2)
project(Aurora-Game-Vk CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/CMakeModules)

set(SOURCES
  src/main.cpp
  src/renderer.cpp
  ../game/src/gltf_loader.cpp
)

set(HEADERS
  src/shader/output.vert.h
  src/shader/output.frag.h
  src/shader/triangle.vert.h
  src/shader/triangle.frag.h
  src/shader/surface.vert.h
  src/shader/surface.frag.h
  src/renderer.hpp
  ../game/src/gltf_loader.hpp
)

include(FindSDL2)
find_package(SDL2 REQUIRED)
find_package(Vulkan REQUIRED)

add_executable(Aurora-Game-Vk ${SOURCES} ${HEADERS})
target_link_libraries(Aurora-Game-Vk PRIVATE Aurora-Renderer ${SDL2_LIBRARY} Aurora-GAL Vulkan::Vulkan nlohmann_json shaderc)
target_include_directories(Aurora-Game-Vk PRIVATE ${SDL2_INCLUDE_DIR})
