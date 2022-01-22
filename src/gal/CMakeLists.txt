cmake_minimum_required(VERSION 3.2)
project(Aurora-GAL CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES
  src/vulkan/render_device.cpp
)

set(HEADERS
  src/vulkan/buffer.hpp
  src/vulkan/common.hpp
  src/vulkan/render_target.hpp
  src/vulkan/shader_module.hpp
  src/vulkan/texture.hpp
)

set(HEADERS_PUBLIC
  include/aurora/gal/backend/vulkan.hpp
  include/aurora/gal/buffer.hpp
  include/aurora/gal/render_device.hpp
  include/aurora/gal/render_target.hpp
  include/aurora/gal/shader_module.hpp
  include/aurora/gal/texture.hpp
)

find_package(Vulkan REQUIRED)

add_library(Aurora-GAL ${SOURCES} ${HEADERS} ${HEADERS_PUBLIC})

target_link_libraries(Aurora-GAL PUBLIC Aurora-Common)
target_link_libraries(Aurora-GAL PRIVATE Vulkan::Vulkan)

target_include_directories(Aurora-GAL PUBLIC include)
target_include_directories(Aurora-GAL PRIVATE src)
