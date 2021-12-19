
add_subdirectory(fmt)

add_library(stb stb.cpp stb/stb_image.h)
target_include_directories(stb INTERFACE stb)