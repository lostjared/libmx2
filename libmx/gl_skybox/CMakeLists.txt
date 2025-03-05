add_executable(gl_skybox skybox.cpp)
target_include_directories(gl_skybox PRIVATE 
    ${CMAKE_SOURCE_DIR}/libmx 
    ${SDL2_INCLUDE_DIRS} 
    ${SDL2_TTF_INCLUDE_DIRS}
)
target_link_libraries(gl_skybox PRIVATE mx ${SDL2_LIBRARIES} ${SDL2_TTF_LIBRARIES})
