
find_package(MXWrite REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)

if(NOT PKG_CONFIG_FOUND)
    message(FATAL_ERROR "pkg-config not found. Please install pkg-config.")
endif()

# Handle FFmpeg with homebrew on macOS
if(APPLE)
    # Try to find homebrew FFmpeg
    execute_process(
        COMMAND brew --prefix ffmpeg
        OUTPUT_VARIABLE HOMEBREW_FFMPEG_PREFIX
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    if(HOMEBREW_FFMPEG_PREFIX)
        message(STATUS "Found homebrew FFmpeg at: ${HOMEBREW_FFMPEG_PREFIX}")
        set(ENV{PKG_CONFIG_PATH} "${HOMEBREW_FFMPEG_PREFIX}/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")
    endif()
endif()

pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET
    libavcodec
    libavformat
    libavutil
    libswscale
)
if(NOT FFMPEG_FOUND)
    message(FATAL_ERROR "FFMPEG libraries not found. Please install ffmpeg development libraries (libavcodec, libavformat, libavutil, libswscale).")
endif()
message(STATUS "FFMPEG found: ${FFMPEG_LIBRARIES}")

add_executable(vk_fractal ${CMAKE_SOURCE_DIR}/vk_fractal/fractal.cpp ${CMAKE_SOURCE_DIR}/vk_fractal/vk.cpp)
target_include_directories(vk_fractal PRIVATE 
    ${CMAKE_SOURCE_DIR}/libmx 
    ${SDL2_INCLUDE_DIRS} 
    ${SDL2_TTF_INCLUDE_DIRS}
    ${NXWRITE_INCLUDE_DIRS}
    ${FFMPEG_INCLUDE_DIRS}
)

target_link_libraries(vk_fractal PUBLIC SDL2::SDL2main mxvk volk MXWrite::mxwrite ${FFMPEG_LIBRARIES} Threads::Threads)

