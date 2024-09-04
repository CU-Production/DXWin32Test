cmake_minimum_required(VERSION 3.28)
project(DXtest)

set(CMAKE_CXX_STANDARD 17)

#add_executable(DXtest main.cpp)
add_executable(DXtest WIN32 main.cpp)
#if (MSVC)
#    target_compile_options(DXtest PRIVATE /W4 /WX /SUBSYSTEM:WINDOWS)
#else ()
#    error("Only support MSVC!!!")
#endif ()

#target_compile_options(${TARGET_NAME} PRIVATE
#    $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
#    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Werror>
#)
