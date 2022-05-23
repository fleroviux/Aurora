
add_subdirectory(fmt)

set(JSON_BuildTests OFF CACHE INTERNAL "")
add_subdirectory(json)

set(SPIRV_HEADERS_SKIP_EXAMPLES ON CACHE INTERNAL "")
set(SPIRV_HEADERS_SKIP_INSTALL ON CACHE INTERNAL "")
add_subdirectory(SPIRV-Headers)

set(SPIRV_SKIP_TESTS ON CACHE INTERNAL "")
set(SKIP_SPIRV_TOOLS_INSTALL ON CACHE INTERNAL "")
set(SPIRV_WERROR OFF CACHE INTERNAL "")
add_subdirectory(SPIRV-Tools)

set(SKIP_GLSLANG_INSTALL ON CACHE INTERNAL "")
set(ENABLE_GLSLANG_BINARIES OFF CACHE INTERNAL "")
set(ENABLE_CTEST OFF CACHE INTERNAL "")
add_subdirectory(glslang)

set(SHADERC_SKIP_INSTALL ON CACHE INTERNAL "")
set(SHADERC_SKIP_TESTS ON CACHE INTERNAL "")
set(SHADERC_SKIP_EXAMPLES ON CACHE INTERNAL "")
set(SHADERC_ENABLE_SHARED_CRT ON CACHE INTERNAL "")
add_subdirectory(shaderc)

add_library(stb stb.cpp stb/stb_image.h )
target_include_directories(stb PUBLIC stb)

add_subdirectory(VulkanMemoryAllocator)
