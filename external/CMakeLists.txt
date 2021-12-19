
add_subdirectory(fmt)

set(JSON_BuildTests OFF CACHE INTERNAL "")
add_subdirectory(json)

add_library(stb stb.cpp stb/stb_image.h)
target_include_directories(stb PUBLIC stb)