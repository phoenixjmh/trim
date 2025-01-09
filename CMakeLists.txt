cmake_minimum_required(VERSION 3.14)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
project(trim C CXX)


# Hint: Helps CMake find packages
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH})


set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(lib/FFMpeg)

set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)

add_subdirectory(lib/glfw)

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


#set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
#set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
#set(CMAKE_INSTALL_RPATH "/home/phoenixjmh/proj/trim/lib/FFMpeg/PlatformLinux/ffmpeg_shared_linux/lib")

add_executable(trim  ${SOURCES})
if(APPLE)
    list(APPEND EXTRA_LIBS
        "-framework OpenGL"
    )


endif()







if(UNIX AND NOT APPLE)

message("CMAKE DIR = ${CMAKE_CURRENT_SOURCE_DIR}")

set(FFMPEG_LIB_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lib/FFMpeg/PlatformLinux/ffmpeg_shared_linux/lib")


link_directories(${FFMPEG_LIB_PATH})
# Include directories


find_library(libswresample_path NAMES libswresample.so.5 PATHS "${FFMPEG_LIB_PATH}/")


find_library(libavcodecpath NAMES libavcodec.so PATHS "/home/phoenixjmh/proj/trim/lib/FFMpeg/PlatformLinux/ffmpeg_shared_linux/lib/")


# Specify linker language
target_link_libraries(trim glfw ${libswresample_path} ${FFMPEG_LIB_PATH}/libavformat.so ${FFMPEG_LIB_PATH}/libavcodec.so ${FFMPEG_LIB_PATH}/libavutil.so  ${FFMPEG_LIB_PATH}/libswscale.so)

set_target_properties(trim PROPERTIES
    BUILD_RPATH "${FFMPEG_LIB_PATH}"  # Path to use during build
    INSTALL_RPATH "${FFMPEG_LIB_PATH}"  # Path to use after installation
    SKIP_BUILD_RPATH FALSE  # Include rpath in the build
    SKIP_INSTALL_RPATH FALSE  # Include rpath in the installed executable
)
endif()


target_include_directories(trim
  PUBLIC
  ${PROJECT_SOURCE_DIR}/vendor/include
  PUBLIC
  lib/FFMpeg/PlatformLinux/ffmpeg_shared_linux/include
  ${PROJECT_BINARY_DIR}
  ${PROJECT_SOURCE_DIR}
)


target_link_libraries(trim glfw FFmpeg)
