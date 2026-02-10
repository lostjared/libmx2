add_executable(vk_matrix_ray ${CMAKE_SOURCE_DIR}/vk_matrix_ray/skeleton.cpp ${CMAKE_SOURCE_DIR}/vk_matrix_ray/vk.cpp)
target_include_directories(vk_matrix_ray PRIVATE 
    ${CMAKE_SOURCE_DIR}/libmx 
    ${SDL2_INCLUDE_DIRS} 
    ${SDL2_TTF_INCLUDE_DIRS}
)

target_link_libraries(vk_matrix_ray PUBLIC SDL2::SDL2main mxvk $<$<NOT:$<BOOL:${MOLTEN}>>:volk>)

