cmake_minimum_required(VERSION 3.14)
find_package(PkgConfig REQUIRED)
project(FFmpeg)
if(WIN32)
    # Set paths relative to this CMakeLists.txt location
    set(FFMPEG_WIN_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/PlatformWindows/ffmpeg_windows")
    set(FFMPEG_LIBRARIES
        "${FFMPEG_WIN_ROOT}/lib/avcodec.lib"
        "${FFMPEG_WIN_ROOT}/lib/avformat.lib"
        "${FFMPEG_WIN_ROOT}/lib/avfilter.lib"
        "${FFMPEG_WIN_ROOT}/lib/avdevice.lib"
        "${FFMPEG_WIN_ROOT}/lib/avutil.lib"
        "${FFMPEG_WIN_ROOT}/lib/swresample.lib"
        "${FFMPEG_WIN_ROOT}/lib/swscale.lib"
    )
    
    # Create interface library
    add_library(FFmpeg INTERFACE)
    target_include_directories(FFmpeg INTERFACE "${FFMPEG_WIN_ROOT}/include")
    target_link_libraries(FFmpeg INTERFACE ${FFMPEG_LIBRARIES})
    
    # Copy DLLs to build directory (3 levels up)
    file(GLOB FFMPEG_DLLS "${FFMPEG_WIN_ROOT}/bin/*.dll")
	#Copy Dlls into the executable directory
	file(COPY ${FFMPEG_DLLS} DESTINATION "${CMAKE_CURRENT_SOURCE_DIR}/../../bin/Debug")
	file(COPY ${FFMPEG_DLLS} DESTINATION "${CMAKE_CURRENT_SOURCE_DIR}/../../bin/Release")
	
else()
# Set PKG_CONFIG_PATH to include the local FFmpeg pkgconfig directory
#message("Building NOT for windows")
#set(PKG_CONFIG_PATH_OLD $ENV{PKG_CONFIG_PATH})
#set(ENV{PKG_CONFIG_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/PlatformLinux/ffmpeg_shared_linux/lib/pkgconfig:${PKG_CONFIG_PATH_OLD}")

#pkg_check_modules(AVCODEC     REQUIRED IMPORTED_TARGET libavcodec)
#pkg_check_modules(AVFORMAT    REQUIRED IMPORTED_TARGET libavformat)
#pkg_check_modules(AVFILTER    REQUIRED IMPORTED_TARGET libavfilter)
#pkg_check_modules(AVDEVICE    REQUIRED IMPORTED_TARGET libavdevice)
#pkg_check_modules(AVUTIL      REQUIRED IMPORTED_TARGET libavutil)
#pkg_check_modules(SWRESAMPLE  REQUIRED IMPORTED_TARGET libswresample)
#pkg_check_modules(SWSCALE     REQUIRED IMPORTED_TARGET libswscale)
#
#add_library(FFmpeg INTERFACE IMPORTED GLOBAL)
#
#target_link_libraries(FFmpeg INTERFACE
#    PkgConfig::AVCODEC
#    PkgConfig::AVFORMAT
#    PkgConfig::AVFILTER
#    PkgConfig::AVDEVICE
#    PkgConfig::AVUTIL
#    PkgConfig::SWRESAMPLE
#    PkgConfig::SWSCALE
#)

endif()
