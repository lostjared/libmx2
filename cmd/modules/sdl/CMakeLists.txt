cmake_minimum_required(VERSION 3.14)
project(sdl-mod VERSION 1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
find_package(SDL2 REQUIRED)
find_package(libmxcmd REQUIRED)
add_library(sdl-mod SHARED sdl-module.cpp)
include(GNUInstallDirs)
target_include_directories(sdl-mod PRIVATE ${SDL2_INCLUDE_DIRS} 
${LIBCMD_INCLUDE_DIRS}
${LIBCMD_INCLUDE_DIRS}/mxcmd
)
target_link_libraries(sdl-mod PRIVATE libmxcmd::mxcmd ${SDL2_LIBRARIES})

install(TARGETS sdl-mod
    LIBRARY DESTINATION ${MODULE_INSTALL_DIR}
    RUNTIME DESTINATION ${MODULE_INSTALL_DIR}
)