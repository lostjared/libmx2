cmake_minimum_required(VERSION 3.10)
project(scanner_cxx VERSION 1.0 LANGUAGES CXX)


set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(scanner SHARED
    scanner.cpp
    string_buffer.cpp
    types.cpp
)

add_executable(scanner_test
    cmd.cpp
)

target_include_directories(scanner_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(scanner_test PRIVATE
    scanner
)

target_include_directories(scanner PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

set_target_properties(scanner PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    PUBLIC_HEADER "scanner.hpp;string_buffer.hpp;types.hpp;parser.hpp;"
)

install(TARGETS scanner
    LIBRARY DESTINATION lib
    PUBLIC_HEADER DESTINATION include
)