add_executable(gl_mxmod viewer.cpp)
target_include_directories(gl_mxmod PRIVATE 
    ${CMAKE_SOURCE_DIR}/libmx 
    ${SDL2_INCLUDE_DIRS} 
    ${SDL2_TTF_INCLUDE_DIRS}
)
target_link_libraries(gl_mxmod PRIVATE mxgl ${SDL2_LIBRARIES} ${SDL2_TTF_LIBRARIES})

# Copy data directory to build output
add_custom_command(TARGET gl_mxmod POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/data
    ${CMAKE_CURRENT_BINARY_DIR}/data
    COMMENT "Copying gl_mxmod data files to build directory"
)

install(TARGETS gl_mxmod
        RUNTIME DESTINATION bin)
install(DIRECTORY data/
        DESTINATION bin/data)