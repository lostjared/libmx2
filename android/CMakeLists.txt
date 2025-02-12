cmake_minimum_required(VERSION 3.10)
project(libmx2_app)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions -frtti")
add_definitions(-DWITH_MIXER)
target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../SDL/include
)

set(SRC_FILES
    main.cpp
    model.cpp
    gl.cpp
    cfg.cpp
    loadpng.cpp
    exception.cpp
    font.cpp
    input.cpp
    joystick.cpp
    mx.cpp
    sound.cpp
    tee_stream.cpp
    texture.cpp
    util.cpp
)

add_library(${PROJECT_NAME} SHARED ${SRC_FILES})


target_link_libraries(${PROJECT_NAME}
    SDL2
    SDL2_ttf
    SDL2_image
    SDL2_mixer

    # Link against Android and GLES libraries
    GLESv1_CM
    GLESv3
    EGL
    OpenSLES
    log
    android

    # These may be required when using the NDK’s libc++ (adjust as needed)
    c++_static
    c++abi
)
