cmake_minimum_required(VERSION 3.28)
project(DXtest)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(3rd_party)

add_executable(DX11win32 WIN32 mainwin32dx11.cpp)

add_executable(DX11glfw mainglfwdx11.cpp)
target_link_libraries(DX11glfw PRIVATE glfw)

add_executable(DX12win32 WIN32 mainwin32dx12.cpp)

add_executable(SlangGlfw mainglfwslang.cpp gfx-util/shader-cursor.cpp gfx-util/shader-cursor.h)
target_link_libraries(SlangGlfw PRIVATE glfw SlangGFX)
add_custom_command(
    TARGET SlangGlfw POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different "${CMAKE_CURRENT_SOURCE_DIR}/3rd_party/slang/bin" "${CMAKE_CURRENT_BINARY_DIR}/"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/triangle.slang" "${CMAKE_CURRENT_BINARY_DIR}/"
    VERBATIM
)
