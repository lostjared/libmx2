add_executable(vk_tex_cube ${CMAKE_SOURCE_DIR}/vk_tex_cube/main.cpp ${CMAKE_SOURCE_DIR}/vk_tex_cube/vk.cpp)
target_include_directories(vk_tex_cube PRIVATE 
    ${CMAKE_SOURCE_DIR}/libmx 
    ${SDL2_INCLUDE_DIRS} 
    ${SDL2_TTF_INCLUDE_DIRS}
)

target_link_libraries(vk_tex_cube PUBLIC SDL2::SDL2main mxvk volk)

