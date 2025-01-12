add_executable(gl_walk walk.cpp)
target_include_directories(gl_walk PRIVATE 
    ${CMAKE_SOURCE_DIR}/libmx 
    ${SDL2_INCLUDE_DIRS} 
    ${SDL2_TTF_INCLUDE_DIRS}
)
target_link_libraries(gl_walk PRIVATE mx ${SDL2_LIBRARIES} ${SDL2_TTF_LIBRARIES})
