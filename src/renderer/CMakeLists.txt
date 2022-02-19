cmake_minimum_required(VERSION 3.2)
project(Aurora-Renderer CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/CMakeModules)

set(SOURCES
  src/renderer.cpp
  src/texture.cpp
)

set(HEADERS
  src/pbr.glsl.hpp
)

set(HEADERS_PUBLIC
  include/aurora/renderer/component/camera.hpp
  include/aurora/renderer/component/mesh.hpp
  include/aurora/renderer/component/scene.hpp
  include/aurora/renderer/geometry/geometry.hpp
  include/aurora/renderer/geometry/index_buffer.hpp
  include/aurora/renderer/geometry/vertex_buffer.hpp
  include/aurora/renderer/gpu_resource.hpp
  include/aurora/renderer/material.hpp
  include/aurora/renderer/renderer.hpp
  include/aurora/renderer/texture.hpp
  include/aurora/renderer/uniform_block.hpp
)

find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)

add_library(Aurora-Renderer ${SOURCES} ${HEADERS} ${HEADERS_PUBLIC})
target_include_directories(Aurora-Renderer PRIVATE ${GLEW_INCLUDE_DIRS})
target_include_directories(Aurora-Renderer PUBLIC include)
target_link_libraries(Aurora-Renderer PUBLIC Aurora-Scene)
target_link_libraries(Aurora-Renderer PRIVATE Aurora-GAL OpenGL::GL GLEW::GLEW stb)
