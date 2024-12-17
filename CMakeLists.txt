cmake_minimum_required(VERSION 3.14)
project(trim C CXX)
set(CMAKE_CXX_STANDARD 14)

# Hint: Helps CMake find packages
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH})

add_subdirectory(lib/FFMpeg)
add_subdirectory(lib/glfw)

find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)

list(APPEND SOURCES
        main.cpp
)

add_executable(trim ${SOURCES})











target_link_libraries(trim
        FFMpeg
        glfw
        GLEW::GLEW
        OpenGL::OpenGL
)
