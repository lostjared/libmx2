add_executable(vk_sprite ${CMAKE_SOURCE_DIR}/vk_sprite/skeleton.cpp ${CMAKE_SOURCE_DIR}/vk_sprite/vk.cpp)
target_include_directories(vk_sprite PRIVATE 
    ${CMAKE_SOURCE_DIR}/libmx 
    ${SDL2_INCLUDE_DIRS} 
    ${SDL2_TTF_INCLUDE_DIRS}
)

target_link_libraries(vk_sprite PUBLIC SDL2::SDL2main mxvk $<$<NOT:$<BOOL:${MOLTEN}>>:volk>)

