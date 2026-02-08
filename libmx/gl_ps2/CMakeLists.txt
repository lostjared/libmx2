add_executable(gl_ps2 ps.cpp)
target_include_directories(gl_ps2 PRIVATE 
    ${CMAKE_SOURCE_DIR}/libmx 
    ${SDL2_INCLUDE_DIRS} 
    ${SDL2_TTF_INCLUDE_DIRS}
)
target_link_libraries(gl_ps2 PRIVATE mxgl ${SDL2_LIBRARIES} ${SDL2_TTF_LIBRARIES})

