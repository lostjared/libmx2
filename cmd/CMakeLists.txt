cmake_minimum_required(VERSION 3.10)
project(cmd_cxx VERSION 1.0 LANGUAGES CXX C)


option(CMD "Build command line interface" OFF)
option(CONSOLE "Build graphical console" OFF)

find_library(Readline NAMES readline)
find_path(Readline_INCLUDE_DIRS NAMES readline/readline.h)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(cmd STATIC
    src/scanner.cpp
    src/string_buffer.cpp
    src/types.cpp
    src/command.cpp
    src/ast.cpp
)

list(APPEND header ${CMAKE_SOURCE_DIR}/include/command.hpp ${CMAKE_SOURCE_DIR}/include/scanner.hpp ${CMAKE_SOURCE_DIR}/include/types.hpp ${CMAKE_SOURCE_DIR}/include/string_buffer.hpp ${CMAKE_SOURCE_DIR}/include/ast.hpp ${CMAKE_SOURCE_DIR}/include/parser.hpp)

include_directories(${PROJECT_SOURCE_DIR}/include)


if(CMD)
    add_executable(cmd-cli src/cli.cpp)
    target_link_libraries(cmd-cli PRIVATE
        cmd
        ${Readline_LIBRARIES}
        readline
    )

endif()


if(CONSOLE)
    find_package(SDL2 REQUIRED)
    find_package(SDL2_ttf REQUIRED)
    find_package(SDL2_mixer REQUIRED)
    find_package(libmx2 REQUIRED)
    add_executable(cmd-console
        src/console_full.cpp
    )
    target_include_directories(cmd-console PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${libmx2_INCLUDE_DIRS}
        ${SDL2_INCLUDE_DIRS} 
        ${SDL2_TTF_INCLUDE_DIRS}
        ${SDL2_MIXER_INCLUDE_DIRS}
    )
    target_link_libraries(cmd-console PRIVATE
        cmd
        libmx2::mx 
        ${SDL2_LIBRARIES} 
        ${SDL2_TTF_LIBRARIES} 
        ${SDL2_MIXER_LIBRARIES}    
    )
endif()

include(GNUInstallDirs)

set_target_properties(cmd PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    PUBLIC_HEADER "${header}"
    OUTPUT_NAME "cmd"  
)


install(TARGETS cmd
    EXPORT cmdTargets  
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}      
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}      
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/cmd  
)

install(EXPORT cmdTargets
    FILE cmdTargets.cmake
    NAMESPACE libcmd::  
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libcmd
)

include(CMakePackageConfigHelpers)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/libcmdConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/libcmdConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/cmd
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/libcmdConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

export(
    TARGETS cmd
    FILE ${CMAKE_CURRENT_BINARY_DIR}/libcmdTargets.cmake
    NAMESPACE libcmd::
)


install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/libcmdConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/libcmdConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libcmd
)

install(
    EXPORT cmdTargets
    FILE libcmdTargets.cmake
    NAMESPACE libcmd::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libcmd
)
