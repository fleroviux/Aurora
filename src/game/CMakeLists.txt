cmake_minimum_required(VERSION 3.2)
project(Aurora-Game CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/CMakeModules)

set(SOURCES
  src/gltf_loader.cpp
  src/main.cpp
)

set(HEADERS
  src/gltf_loader.hpp
  src/texture.hpp
)

include(FindSDL2)
find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)

add_executable(Aurora-Game ${SOURCES} ${HEADERS})
target_link_libraries(Aurora-Game PRIVATE Aurora-Scene ${SDL2_LIBRARY} OpenGL::GL GLEW::GLEW stb nlohmann_json::nlohmann_json)
target_include_directories(Aurora-Game PRIVATE ${SDL2_INCLUDE_DIR} ${GLEW_INCLUDE_DIRS})
