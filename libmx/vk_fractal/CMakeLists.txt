
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
    ${CMAKE_SOURCE_DIR}/volk
    ${SDL2_INCLUDE_DIRS} 
    ${SDL2_TTF_INCLUDE_DIRS}
    ${VK_INCLUDE}
    ${NXWRITE_INCLUDE_DIRS}
    ${FFMPEG_INCLUDE_DIRS}
)

if(MOLTEN)
    target_link_directories(vk_fractal PRIVATE ${MOLTEN_LINK_DIR})
    target_link_libraries(vk_fractal PRIVATE ${MOLTEN_FRAMEWORKS})
    target_link_libraries(vk_fractal PUBLIC MXWrite::mxwrite SDL2::SDL2 SDL2::SDL2main SDL2_ttf::SDL2_ttf mx ${IMG_LIB} ${GL_LIB} ${MIXER_LIB} ${VK_LIB} ${FFMPEG_LIBRARIES} Threads::Threads)
else()
    target_link_libraries(vk_fractal PUBLIC MXWrite::mxwrite SDL2::SDL2 SDL2::SDL2main SDL2_ttf::SDL2_ttf mx volk ${IMG_LIB} ${GL_LIB} ${MIXER_LIB} ${VK_LIB} ${FFMPEG_LIBRARIES} Threads::Threads)
endif()

