add_executable(gl_animation animation.cpp)
target_include_directories(gl_animation PRIVATE 
    ${CMAKE_SOURCE_DIR}/libmx 
    ${SDL2_INCLUDE_DIRS} 
    ${SDL2_TTF_INCLUDE_DIRS}
)
target_link_libraries(gl_animation PRIVATE mx ${SDL2_LIBRARIES} ${SDL2_TTF_LIBRARIES})
