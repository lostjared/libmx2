add_executable(gl_stars3d stars.cpp)
target_include_directories(gl_stars3d PRIVATE 
    ${CMAKE_SOURCE_DIR}/libmx 
    ${SDL2_INCLUDE_DIRS} 
    ${SDL2_TTF_INCLUDE_DIRS}
)
target_link_libraries(gl_stars3d PRIVATE mxgl ${SDL2_LIBRARIES} ${SDL2_TTF_LIBRARIES})


