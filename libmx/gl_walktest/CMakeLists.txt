add_executable(gl_walktest room.cpp)
target_include_directories(gl_walktest PRIVATE 
    ${CMAKE_SOURCE_DIR}/libmx 
    ${SDL2_INCLUDE_DIRS} 
    ${SDL2_TTF_INCLUDE_DIRS}
)
target_link_libraries(gl_walktest PRIVATE mx ${SDL2_LIBRARIES} ${SDL2_TTF_LIBRARIES})
