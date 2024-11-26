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
if(JPEG)
find_package(JPEG REQUIRED)
message(STATUS "Added JPEG Support")
message(STATUS "JPEG Libraries: ${JPEG_LIBRARIES}")
message(STATUS "JPEG Include Dirs: ${JPEG_INCLUDE_DIRS}")
add_compile_definitions(WITH_JPEG)    
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
if(JPEG)
add_subdirectory(jpeg_test)
endif()
else()
message(STATUS "Not Building Examples")
endif()

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
