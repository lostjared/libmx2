add_executable(gl_matrix_room matrix_room.cpp)
target_include_directories(gl_matrix_room PRIVATE 
    ${CMAKE_SOURCE_DIR}/libmx 
    ${SDL2_INCLUDE_DIRS} 
    ${SDL2_TTF_INCLUDE_DIRS}
)
target_link_libraries(gl_matrix_room PRIVATE mx ${SDL2_LIBRARIES} ${SDL2_TTF_LIBRARIES})
