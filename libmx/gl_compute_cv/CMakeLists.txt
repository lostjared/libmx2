add_executable(gl_compute_cv example.cpp)
target_include_directories(gl_compute_cv PRIVATE 
    ${CMAKE_SOURCE_DIR}/libmx 
    ${SDL2_INCLUDE_DIRS} 
    ${SDL2_TTF_INCLUDE_DIRS}
)
target_link_libraries(gl_compute_cv PRIVATE mxgl ${SDL2_LIBRARIES} ${SDL2_TTF_LIBRARIES})
