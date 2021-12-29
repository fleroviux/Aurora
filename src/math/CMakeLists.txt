cmake_minimum_required(VERSION 3.2)
project(Aurora-Math CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(HEADERS_PUBLIC
  include/aurora/math/matrix4.hpp
  include/aurora/math/numeric_traits.hpp
  include/aurora/math/quaternion.hpp
  include/aurora/math/rotation.hpp
  include/aurora/math/vector.hpp
)

add_library(Aurora-Math INTERFACE ${HEADERS_PUBLIC})
target_include_directories(Aurora-Math INTERFACE include)
target_link_libraries(Aurora-Math INTERFACE Aurora-Common)
