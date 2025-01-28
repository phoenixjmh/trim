cmake_minimum_required(VERSION 3.14)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
project(trim C CXX)


# Hint: Helps CMake find packages
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH})

set (CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g") 
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g")



add_subdirectory(lib/FFMpeg EXCLUDE_FROM_ALL)



set(ALSOFT_REQUIRE_REBUILD_VERSION FALSE CACHE BOOL "Force version.h rebuild" FORCE)
add_subdirectory(lib/OpenAL EXCLUDE_FROM_ALL)


if(TARGET alsoft.build_version)
          message("OH THATS JUST GREAT")
    set_target_properties(alsoft.build_version PROPERTIES
        EXCLUDE_FROM_DEFAULT_BUILD TRUE
        EXCLUDE_FROM_ALL TRUE)
endif()



set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)


add_subdirectory(lib/glfw)

add_definitions(-DGL_SILENCE_DEPRECATION)

list(APPEND SOURCES
        src/main.cpp
        src/gui.cpp
        src/video_operations.cpp

       
)

list(APPEND VENDOR_SRC
        vendor/src/glad.c
        vendor/src/imgui.cpp
        vendor/src/imgui_demo.cpp
        vendor/src/imgui_draw.cpp
        vendor/src/imgui_impl_glfw.cpp
        vendor/src/imgui_impl_opengl3.cpp
        vendor/src/imgui_tables.cpp
        vendor/src/imgui_widgets.cpp
)

list(APPEND HEADERS
    src/trim.h
    src/gui.h
    src/video_operations.h
    src/common_structs.h
    src/range_slider.h
    src/helpers.h


)

source_group("Vendor" FILES ${VENDOR_SRC})
source_group("Sources" FILES ${SOURCES})
source_group("Headers" FILES ${HEADERS})



#set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
#set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
#set(CMAKE_INSTALL_RPATH "/home/phoenixjmh/proj/trim/lib/FFMpeg/PlatformLinux/ffmpeg_shared_linux/lib")

add_executable(trim  ${SOURCES} ${VENDOR_SRC} ${HEADERS}
        src/image_loader.cpp
        src/image_loader.h
        vendor/include/stbi_image.h)
if(APPLE)
    list(APPEND EXTRA_LIBS
        "-framework OpenGL"
    )


endif()


set(OPEN_AL_LIB_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lib/OpenAL")





if(UNIX AND NOT APPLE)

      message("CMAKE DIR = ${CMAKE_CURRENT_SOURCE_DIR}")

      set(FFMPEG_LIB_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lib/FFMpeg/PlatformLinux/ffmpeg_shared_linux/lib")


      link_directories(${FFMPEG_LIB_PATH})

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


target_link_libraries(
      trim
      glfw
      FFmpeg
      OpenAL
     # ${OPEN_AL_LIB_PATH}/OpenAL32.lib
      )
if(WIN32)
    add_custom_command(TARGET trim POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${OPEN_AL_LIB_PATH}/OpenAL32.dll"
        $<TARGET_FILE_DIR:trim>
    )
endif()
