add_executable(gl_glitch_cube3 glitch.cpp)
target_include_directories(gl_glitch_cube3 PRIVATE 
    ${CMAKE_SOURCE_DIR}/libmx 
    ${SDL2_INCLUDE_DIRS} 
    ${SDL2_TTF_INCLUDE_DIRS}
    /usr/include
    /usr/local/include
)
target_link_libraries(gl_glitch_cube3 PRIVATE mx ${SDL2_LIBRARIES} ${SDL2_TTF_LIBRARIES})
