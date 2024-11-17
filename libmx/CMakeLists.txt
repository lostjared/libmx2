cmake_minimum_required(VERSION 3.10)
project(libmx_project)
find_package(SDL2 REQUIRED)
find_package(SDL2_ttf REQUIRED)

set(SDL2_INCLUDE_DIRS ${SDL2_INCLUDE_DIRS})
set(SDL2_LIBRARIES ${SDL2_LIBRARIES})
set(SDL2_TTF_INCLUDE_DIRS ${SDL2_TTF_INCLUDE_DIRS})
set(SDL2_TTF_LIBRARIES SDL2_ttf::SDL2_ttf)

add_subdirectory(libmx)
add_subdirectory(test)
