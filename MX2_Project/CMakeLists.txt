cmake_minimum_required(VERSION 3.10)
project(ACMX2 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL2 REQUIRED)
find_package(SDL2_ttf REQUIRED)
find_package(SDL2_mixer REQUIRED)
find_package(libmx2 REQUIRED)

add_executable(mx2_app main.cpp)

target_compile_options(mx2_app PRIVATE -O2 -Wall -pedantic)

target_include_directories(mx2_app PRIVATE 
    ${libmx2_INCLUDE_DIRS}
    ${SDL2_INCLUDE_DIRS} 
    ${SDL2_TTF_INCLUDE_DIRS}
    ${SDL2_MIXER_INCLUDE_DIRS}
    /usr/include
    /usr/local/include
    /opt/homebrew/include
    ${CMAKE_PREFIX_PATH}
)

target_link_libraries(mx2_app PRIVATE 
    libmx2::mx 
    ${SDL2_LIBRARIES} 
    ${SDL2_TTF_LIBRARIES} 
    ${SDL2_MIXER_LIBRARIES}
)

install(TARGETS mx2_app DESTINATION bin)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/webmx/mx2/include/ DESTINATION include)

