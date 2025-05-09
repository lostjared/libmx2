find_package(libmxcmd REQUIRED)


add_executable(mp3d main.cpp quadtris.cpp intro.cpp start.cpp gameover.cpp)
target_include_directories(mp3d PRIVATE 
    ${CMAKE_SOURCE_DIR}/libmx 
    ${SDL2_INCLUDE_DIRS} 
    ${SDL2_TTF_INCLUDE_DIRS}
    ${LIBCMD_INCLUDE_DIRS}
    ${LIBCMD_INCLUDE_DIRS}/mxcmd
)
target_link_libraries(mp3d PRIVATE mx libmxcmd::mxcmd ${SDL2_LIBRARIES} ${SDL2_TTF_LIBRARIES})
