# GLFW
set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "shared")
set(GLFW_BUILD_EXAMPLES OFF CACHE INTERNAL "glfw examples")
set(GLFW_BUILD_TESTS OFF CACHE INTERNAL "glfw tests")
set(GLFW_BUILD_DOCS OFF CACHE INTERNAL "glfw docs")
set(GLFW_INSTALL OFF CACHE INTERNAL "glfw install")
set(GLFW_VULKAN_STATIC OFF CACHE INTERNAL "glfw vulkan")
add_subdirectory(glfw)

# Slang & Slang-gfx
add_library(SlangGFX INTERFACE)
target_include_directories(SlangGFX INTERFACE slang/include)
target_link_directories(SlangGFX INTERFACE slang/lib)
target_link_libraries(SlangGFX INTERFACE slang gfx)
