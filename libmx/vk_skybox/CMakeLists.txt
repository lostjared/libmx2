add_executable(vk_skybox main.cpp ${CMAKE_CURRENT_SOURCE_DIR}/vk.cpp)

target_include_directories(vk_skybox PRIVATE 
    ${CMAKE_SOURCE_DIR}/libmx 
    ${SDL2_INCLUDE_DIRS} 
    ${SDL2_TTF_INCLUDE_DIRS}
)

target_link_libraries(vk_skybox PUBLIC SDL2::SDL2main mxvk $<$<NOT:$<BOOL:${MOLTEN}>>:volk>)

# Copy data files to build directory
add_custom_command(TARGET vk_skybox POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/data
    ${CMAKE_CURRENT_BINARY_DIR}/data
    COMMENT "Copying vk_skybox data files"
)
