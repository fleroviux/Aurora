cmake_minimum_required(VERSION 3.2)
project(Aurora-Scene CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES
  src/component/transform.cpp
)

set(HEADERS
)

set(HEADERS_PUBLIC
  include/aurora/scene/component/transform.hpp
  include/aurora/scene/geometry/geometry.hpp
  include/aurora/scene/geometry/index_buffer.hpp
  include/aurora/scene/geometry/vertex_buffer.hpp
  include/aurora/scene/component.hpp
  include/aurora/scene/game_object.hpp
)

add_library(Aurora-Scene ${SOURCES} ${HEADERS} ${HEADERS_PUBLIC})
target_include_directories(Aurora-Scene PUBLIC include)
target_link_libraries(Aurora-Scene PUBLIC Aurora-Math)