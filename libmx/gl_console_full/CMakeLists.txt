add_executable(gl_console_full console_full.cpp)
target_include_directories(gl_console_full PRIVATE 
    ${CMAKE_SOURCE_DIR}/libmx 
    ${SDL2_INCLUDE_DIRS} 
    ${SDL2_TTF_INCLUDE_DIRS}
)
target_link_libraries(gl_console_full PRIVATE mx ${SDL2_LIBRARIES} ${SDL2_TTF_LIBRARIES})
