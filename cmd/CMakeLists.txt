cmake_minimum_required(VERSION 3.10)
project(scanner_cxx VERSION 1.0 LANGUAGES CXX C)


option(CMD "Build the command-line executable" OFF)
option(CONSOLE "Build the MX2 console app" OFF)

find_library(Readline NAMES readline)
find_path(Readline_INCLUDE_DIRS NAMES readline/readline.h)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(scanner STATIC
    src/scanner.cpp
    src/string_buffer.cpp
    src/types.cpp
    src/command.cpp
)

include_directories(${PROJECT_SOURCE_DIR}/include)

if(CMD)
    add_executable(cmd-cli
        src/cmd.cpp
    )

    target_include_directories(cmd-cli PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
	    ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${Readline_INCLUDE_DIRS}
    )
    
    target_link_libraries(cmd-cli PRIVATE
        scanner
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
        /usr/include
        /usr/local/include
        /opt/homebrew/include
        ${CMAKE_PREFIX_PATH}
    )
    target_link_libraries(cmd-console PRIVATE
        scanner
        libmx2::mx 
        ${SDL2_LIBRARIES} 
        ${SDL2_TTF_LIBRARIES} 
        ${SDL2_MIXER_LIBRARIES}    
    )
endif()

target_include_directories(scanner PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include

)

set_target_properties(scanner PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    PUBLIC_HEADER "scanner.hpp;string_buffer.hpp;types.hpp;parser.hpp;command.hpp;ast.hpp"
)

install(TARGETS scanner
    LIBRARY DESTINATION lib
    PUBLIC_HEADER DESTINATION include
)
