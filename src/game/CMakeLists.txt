cmake_minimum_required(VERSION 3.2)
project(Aurora-Game CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES
  src/main.cpp
)

set(HEADERS
)

add_executable(Aurora-Game ${SOURCES} ${HEADERS})
target_link_libraries(Aurora-Game PRIVATE Aurora-Scene)
