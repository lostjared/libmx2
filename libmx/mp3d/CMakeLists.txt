cmake_minimum_required(VERSION 3.10)
project(MasterPiece3D)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(OpenGL_GL_PREFERENCE "GLVND")
if(NOT TARGET libmx2::mx)
    find_package(libmx2 REQUIRED)
endif()
find_package(libmxcmd REQUIRED)
find_package(SDL2 REQUIRED)
find_package(SDL2_ttf REQUIRED)
find_package(SDL2_mixer REQUIRED)

add_executable(mp3d main.cpp quadtris.cpp intro.cpp start.cpp gameover.cpp)
target_include_directories(mp3d PRIVATE 
    ${CMAKE_SOURCE_DIR}/libmx 
    ${SDL2_INCLUDE_DIRS} 
    ${SDL2_TTF_INCLUDE_DIRS}
    ${LIBCMD_INCLUDE_DIRS}
    ${LIBCMD_INCLUDE_DIRS}/mxcmd
)
target_link_libraries(mp3d PRIVATE libmx2::mxgl libmxcmd::mxcmd ${SDL2_LIBRARIES} ${SDL2_TTF_LIBRARIES})
