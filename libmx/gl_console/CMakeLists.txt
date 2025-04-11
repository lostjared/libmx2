add_executable(gl_console console_main.cpp console.cpp)
target_include_directories(gl_console PRIVATE 
    ${CMAKE_SOURCE_DIR}/libmx 
    ${SDL2_INCLUDE_DIRS} 
    ${SDL2_TTF_INCLUDE_DIRS}
)
target_link_libraries(gl_console PRIVATE mx ${SDL2_LIBRARIES} ${SDL2_TTF_LIBRARIES})
