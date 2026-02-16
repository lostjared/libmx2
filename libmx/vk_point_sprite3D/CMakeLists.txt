add_executable(vk_point_sprite3D ${CMAKE_SOURCE_DIR}/vk_point_sprite3D/skeleton.cpp)
target_include_directories(vk_point_sprite3D PRIVATE 
    ${CMAKE_SOURCE_DIR}/libmx 
    ${SDL2_INCLUDE_DIRS} 
    ${SDL2_TTF_INCLUDE_DIRS}
)

target_link_libraries(vk_point_sprite3D PUBLIC SDL2::SDL2main mxvk $<$<NOT:$<BOOL:${MOLTEN}>>:volk>)

