cmake_minimum_required(VERSION 3.16)
project(ollama_gen)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(LIBRARY_NAME ollama_gen)
set(SOURCES mx2-ollama.cpp)
set(HEADERS mx2-ollama.hpp)

find_package(PkgConfig REQUIRED)
pkg_check_modules(CURL REQUIRED libcurl)

add_library(${LIBRARY_NAME} STATIC ${SOURCES} ${HEADERS})
target_include_directories(${LIBRARY_NAME} PRIVATE ${CURL_INCLUDE_DIRS})

target_include_directories(${LIBRARY_NAME} PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

target_compile_options(${LIBRARY_NAME} PRIVATE ${CURL_CFLAGS_OTHER})
target_link_libraries(${LIBRARY_NAME} ${CURL_LIBRARIES})

install(TARGETS ${LIBRARY_NAME}
    EXPORT ollama_gen-targets 
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)
install(FILES ${HEADERS} DESTINATION include)

install(EXPORT ollama_gen-targets
    FILE ollama_gen-targets.cmake
    DESTINATION lib/cmake/ollama_gen
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/ollama_gen-config-version.cmake"
    VERSION 1.0.0
    COMPATIBILITY AnyNewerVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/ollama_gen-config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/ollama_gen-config.cmake"
    INSTALL_DESTINATION lib/cmake/ollama_gen
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/ollama_gen-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/ollama_gen-config-version.cmake"
    DESTINATION lib/cmake/ollama_gen
)