cmake_minimum_required(VERSION 3.2)
project(Aurora-GAL CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES
  src/vulkan/render_device.cpp
)

set(HEADERS
  src/vulkan/bind_group.hpp
  src/vulkan/buffer.hpp
  src/vulkan/command_buffer.hpp
  src/vulkan/command_pool.hpp
  src/vulkan/fence.hpp
  src/vulkan/pipeline_builder.hpp
  src/vulkan/pipeline_layout.hpp
  src/vulkan/queue.hpp
  src/vulkan/render_pass.hpp
  src/vulkan/render_pass_builder.hpp
  src/vulkan/render_target.hpp
  src/vulkan/sampler.hpp
  src/vulkan/shader_module.hpp
  src/vulkan/texture.hpp
  src/vulkan/texture_view.hpp
)

set(HEADERS_PUBLIC
  include/aurora/gal/backend/vulkan.hpp
  include/aurora/gal/bind_group.hpp
  include/aurora/gal/buffer.hpp
  include/aurora/gal/command_buffer.hpp
  include/aurora/gal/command_pool.hpp
  include/aurora/gal/enums.hpp
  include/aurora/gal/fence.hpp
  include/aurora/gal/pipeline_builder.hpp
  include/aurora/gal/pipeline_layout.hpp
  include/aurora/gal/queue.hpp
  include/aurora/gal/render_device.hpp
  include/aurora/gal/render_pass.hpp
  include/aurora/gal/render_target.hpp
  include/aurora/gal/sampler.hpp
  include/aurora/gal/shader_module.hpp
  include/aurora/gal/texture.hpp
)

find_package(Vulkan REQUIRED)

add_library(Aurora-GAL ${SOURCES} ${HEADERS} ${HEADERS_PUBLIC})

target_link_libraries(Aurora-GAL PUBLIC Aurora-Common)
target_link_libraries(Aurora-GAL PRIVATE Vulkan::Vulkan VulkanMemoryAllocator)

target_include_directories(Aurora-GAL PUBLIC include)
target_include_directories(Aurora-GAL PRIVATE src)
