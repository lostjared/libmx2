add_executable(gl_skybox3 skybox.cpp)
target_include_directories(gl_skybox3 PRIVATE 
    ${CMAKE_SOURCE_DIR}/libmx 
    ${SDL2_INCLUDE_DIRS} 
    ${SDL2_TTF_INCLUDE_DIRS}
)
target_link_libraries(gl_skybox3 PRIVATE mx ${SDL2_LIBRARIES} ${SDL2_TTF_LIBRARIES})
