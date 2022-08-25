project(Aurora-Common CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES
)

set(HEADERS
)

set(HEADERS_PUBLIC
  include/aurora/any_ptr.hpp
  include/aurora/array_view.hpp
  include/aurora/integer.hpp
  include/aurora/log.hpp
  include/aurora/result.hpp
  include/aurora/utility.hpp
)

add_library(Aurora-Common INTERFACE ${SOURCES} ${HEADERS} ${HEADERS_PUBLIC})
target_include_directories(Aurora-Common INTERFACE include)
target_link_libraries(Aurora-Common INTERFACE fmt)
