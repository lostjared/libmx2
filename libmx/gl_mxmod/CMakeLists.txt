add_executable(gl_mxmod viewer.cpp)
target_include_directories(gl_mxmod PRIVATE 
    ${CMAKE_SOURCE_DIR}/libmx 
    ${SDL2_INCLUDE_DIRS} 
    ${SDL2_TTF_INCLUDE_DIRS}
)
target_link_libraries(gl_mxmod PRIVATE mx ${SDL2_LIBRARIES} ${SDL2_TTF_LIBRARIES})
