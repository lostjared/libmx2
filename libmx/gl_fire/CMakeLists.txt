add_executable(gl_fire fire.cpp)
target_include_directories(gl_fire PRIVATE 
    ${CMAKE_SOURCE_DIR}/libmx 
    ${SDL2_INCLUDE_DIRS} 
    ${SDL2_TTF_INCLUDE_DIRS}
)
target_link_libraries(gl_fire PRIVATE mx ${SDL2_LIBRARIES} ${SDL2_TTF_LIBRARIES})
