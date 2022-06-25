cmake_minimum_required(VERSION 3.2)
project(Aurora-Renderer CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/CMakeModules)

set(SOURCES
  src/cache/geometry_cache.cpp
  src/effect/ssr/ssr_effect.cpp
  src/forward/forward_render_pipeline.cpp
  src/render_engine.cpp
  src/texture.cpp
)

set(HEADERS
  src/cache/geometry_cache.hpp
  src/effect/ssr/shader/raytrace.glsl.hpp
  src/effect/ssr/ssr_effect.hpp
  src/forward/forward_render_pipeline.hpp
  src/render_pipeline_base.hpp
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
  include/aurora/renderer/render_engine.hpp
  include/aurora/renderer/texture.hpp
  include/aurora/renderer/uniform_block.hpp
)

# TODO: do not find and link Vulkan once it has been fully abstracted.
find_package(Vulkan REQUIRED)

add_library(Aurora-Renderer ${SOURCES} ${HEADERS} ${HEADERS_PUBLIC})
target_include_directories(Aurora-Renderer PRIVATE src ${GLEW_INCLUDE_DIRS})
target_include_directories(Aurora-Renderer PUBLIC include)
target_link_libraries(Aurora-Renderer PUBLIC Aurora-Scene)
target_link_libraries(Aurora-Renderer PRIVATE Aurora-GAL stb shaderc Vulkan::Vulkan)
