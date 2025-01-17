cmake_minimum_required(VERSION 3.10)
project(libmx2 VERSION 1.0.0)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -Wall -pedantic")
find_package(SDL2 REQUIRED)
find_package(SDL2_ttf REQUIRED)
find_package(ZLIB REQUIRED)
find_package(PNG REQUIRED)

set(PNG_LIBS ${PNG_LIBRARIES} ${ZLIB_LIBRARIES})
set(PNG_DIRS ${ZLIB_INCLUDE_DIRS} ${PNG_INCLUDE_DIRS})
set(SDL2_INCLUDE_DIRS ${SDL2_INCLUDE_DIRS})
set(SDL2_LIBRARIES ${SDL2_LIBRARIES})
set(SDL2_TTF_INCLUDE_DIRS ${SDL2_TTF_INCLUDE_DIRS})
set(SDL2_TTF_LIBRARIES SDL2_ttf::SDL2_ttf)
include(GNUInstallDirs)
set(INSTALL_INCLUDE_DIR ${CMAKE_INSTALL_INCLUDEDIR}/mx2)
set(INSTALL_LIB_DIR ${CMAKE_INSTALL_LIBDIR})

option(EXAMPLES "Build Example Projects" ON)
option(JPEG, "Build with JPEG Support" OFF)
option(OPENGL "Build with OpenGL Support" ON)
option(MIXER "Build with Sound Support" ON)

include_directories(${CMAKE_BINARY_DIR} ${CMAKE_INSTALL_PREFIX}/include)

if(MIXER)
find_package(SDL2_mixer REQUIRED)
set (MIXER_INCLUDE ${SDL2_MIXER_INCLUDE_DIRS})
set (MIXER_LIB SDL2_mixer::SDL2_mixer)
message(STATUS "Added SDL_mixer support")
message(STATUS "MIXER_INCLUDE: ${MIXER_INCLUDE}")
message(STATUS "MIXER_LIB: ${MIXER_LIB}")
#add_compile_definitions(WITH_MIXER)
else()
message(STATUS "SDL_mixer disabled.")
endif()

if(OPENGL)
find_package(OpenGL REQUIRED)
find_package(glm REQUIRED)
set (GL_INCLUDE ${OpenGL_INCLUDE_DIRS} ${GLM_INCLUDE_DIRS})
set (GL_LIB  ${OPENGL_gl_LIBRARY})
message (STATUS "Added OpenGL/GLM Support")
message (STATUS "GL INCLUDE: ${GL_INCLUDE}")
message (STATUS "GL LIB: ${GL_LIB}")
#add_compile_definitions(WITH_GL)  
else()
message (STATUS "OpenGL Support Disabled.")
endif()

if(JPEG)
find_package(JPEG REQUIRED)
message(STATUS "Added JPEG Support")
message(STATUS "JPEG Libraries: ${JPEG_LIBRARIES}")
message(STATUS "JPEG Include Dirs: ${JPEG_INCLUDE_DIRS}")
#add_compile_definitions(WITH_JPEG)    
set(IMG_INCLUDE ${JPEG_INCLUDE_DIRS} ${ZLIB_INCLUDE_DIRS} ${PNG_INCLUDE_DIRS})
set(IMG_LIB ${JPEG_LIBRARIES} ${PNG_LIBRARIES} ${ZLIB_LIBRARIES})
else()
message(STATUS "JPEG Disabled")
set(IMG_LIB ${PNG_LIBRARIES} ${ZLIB_LIBRARIES})
set(IMG_INCLUDE ${ZLIB_INCLUDE_DIRS} ${PNG_INCLUDE_DIRS})
endif()

add_subdirectory(libmx)

if(EXAMPLES)
message(STATUS "Building with Example Projects")
add_subdirectory(test)
add_subdirectory(controller_test)
add_subdirectory(input_test)
add_subdirectory(image_test)
add_subdirectory(knights_tour)
add_subdirectory(pong)
add_subdirectory(asteroids)
add_subdirectory(blocks)
add_subdirectory(piece)
add_subdirectory(matrix)
add_subdirectory(breakout)
add_subdirectory(frogger)
add_subdirectory(mad)
add_subdirectory(space)
if(MIXER)
add_subdirectory(sound_test)
endif()

if(JPEG)
add_subdirectory(jpeg_test)
endif()
if(OPENGL)
add_subdirectory(gl_window)
add_subdirectory(gl_skeleton)
add_subdirectory(gl_example)
add_subdirectory(gl_cube)
add_subdirectory(gl_pong)
add_subdirectory(gl_animated_cube)
add_subdirectory(gl_breakout)
add_subdirectory(gl_pyramid)
add_subdirectory(gl_mxmod)
add_subdirectory(space3d)
add_subdirectory(gl_surface_test)
#add_subdirectory(gl_animation)
add_subdirectory(gl_ps)
add_subdirectory(gl_ps2)
add_subdirectory(gl_ps3)
add_subdirectory(gl_ps4)
add_subdirectory(gl_ps3d)
add_subdirectory(gl_stars3d)
add_subdirectory(gl_explode)
add_subdirectory(gl_walk)
endif()
else()
message(STATUS "Not Building Examples")
endif()


if(MIXER)
set(WITH_MIXER_VAL 1)
else()
set(WITH_MIXER_VAL 0)
endif()

if(OPENGL)
set(WITH_OPENGL_VAL 1)
else()
set(WITH_OPENGL_VAL 0)
endif()

if(JPEG)
set(WITH_JPEG_VAL 1)
else()
set(WITH_JPEG_VAL 0)
endif()

configure_file(
    ${CMAKE_SOURCE_DIR}/config.h.in
    ${CMAKE_BINARY_DIR}/config.h
)


include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/libmx2ConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/libmx2Config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/libmx2Config.cmake"
    @ONLY
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/libmx2Config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/libmx2ConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libmx2
)

