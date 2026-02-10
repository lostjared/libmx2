add_executable(vk_tex_pyramid ${CMAKE_SOURCE_DIR}/vk_tex_pyramid/main.cpp ${CMAKE_SOURCE_DIR}/vk_tex_pyramid/vk.cpp)
target_include_directories(vk_tex_pyramid PRIVATE 
    ${CMAKE_SOURCE_DIR}/libmx 
    ${SDL2_INCLUDE_DIRS} 
    ${SDL2_TTF_INCLUDE_DIRS}
)

target_link_libraries(vk_tex_pyramid PUBLIC SDL2::SDL2main mxvk $<$<NOT:$<BOOL:${MOLTEN}>>:volk>)

