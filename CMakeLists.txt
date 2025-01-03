cmake_minimum_required(VERSION 3.14)
project(trim C CXX)
set(CMAKE_CXX_STANDARD 14)

# Hint: Helps CMake find packages
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH})


set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(vendor/lib/FFMpeg)
add_subdirectory(vendor/lib/glfw)

add_definitions(-DGL_SILENCE_DEPRECATION)

list(APPEND SOURCES
        main.cpp
        gui.cpp
        video_operations.cpp

        vendor/src/glad.c
        vendor/src/imgui.cpp
        vendor/src/imgui_demo.cpp
        vendor/src/imgui_draw.cpp
        vendor/src/imgui_impl_glfw.cpp
        vendor/src/imgui_impl_opengl3.cpp
        vendor/src/imgui_tables.cpp
        vendor/src/imgui_widgets.cpp
)

add_executable(trim  ${SOURCES})

    list(APPEND EXTRA_LIBS
        "-framework OpenGL"
    )

target_include_directories(trim
  PUBLIC
  ${PROJECT_SOURCE_DIR}/vendor/include
)





target_link_libraries(trim FFmpeg glfw ${EXTRA_LIBS})









