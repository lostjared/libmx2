add_executable(gl_blank_model blank_model.cpp)
target_include_directories(gl_blank_model PRIVATE 
    ${CMAKE_SOURCE_DIR}/libmx 
    ${SDL2_INCLUDE_DIRS} 
    ${SDL2_TTF_INCLUDE_DIRS}
)
target_link_libraries(gl_blank_model PRIVATE mx ${SDL2_LIBRARIES} ${SDL2_TTF_LIBRARIES})
