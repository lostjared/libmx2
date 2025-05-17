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

if(NOT DEFINED MODULE_INSTALL_DIR)
  set(MODULE_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/lib")
endif()

if(RELEASE)
    set(CMAKE_BUILD_TYPE Release)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3")
else()
    set(CMAKE_BUILD_TYPE Debug)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")
endif()

if(WIN32)
    set_target_properties(sdl-mod PROPERTIES 
        PREFIX ""  
    )
else()
    set_target_properties(sdl-mod PROPERTIES 
        INSTALL_RPATH "${MODULE_INSTALL_DIR}"
        BUILD_WITH_INSTALL_RPATH TRUE
    )
endif()

target_link_libraries(sdl-mod PRIVATE libmxcmd::mxcmd ${SDL2_LIBRARIES})

install(TARGETS sdl-mod
    LIBRARY DESTINATION ${MODULE_INSTALL_DIR}
    RUNTIME DESTINATION ${MODULE_INSTALL_DIR}
)