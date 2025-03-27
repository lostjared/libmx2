add_executable(gl_model_glitch model_glitch.cpp)
target_include_directories(gl_model_glitch PRIVATE 
    ${CMAKE_SOURCE_DIR}/libmx 
    ${SDL2_INCLUDE_DIRS} 
    ${SDL2_TTF_INCLUDE_DIRS}
)
target_link_libraries(gl_model_glitch PRIVATE mx ${SDL2_LIBRARIES} ${SDL2_TTF_LIBRARIES})
