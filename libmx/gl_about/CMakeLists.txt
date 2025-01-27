add_executable(gl_about about.cpp)
target_include_directories(gl_about PRIVATE 
    ${CMAKE_SOURCE_DIR}/libmx 
    ${SDL2_INCLUDE_DIRS} 
    ${SDL2_TTF_INCLUDE_DIRS}
)
target_link_libraries(gl_about PRIVATE mx ${SDL2_LIBRARIES} ${SDL2_TTF_LIBRARIES})
