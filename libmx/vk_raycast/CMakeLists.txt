add_executable(vk_raycast ${CMAKE_SOURCE_DIR}/vk_raycast/skeleton.cpp ${CMAKE_SOURCE_DIR}/vk_raycast/vk.cpp)
target_include_directories(vk_raycast PRIVATE 
    ${CMAKE_SOURCE_DIR}/libmx 
    ${SDL2_INCLUDE_DIRS} 
    ${SDL2_TTF_INCLUDE_DIRS}
)

target_link_libraries(vk_raycast PUBLIC SDL2::SDL2main mxvk $<$<NOT:$<BOOL:${MOLTEN}>>:volk>)

