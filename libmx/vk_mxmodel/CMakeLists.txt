add_executable(vk_mxmodel ${CMAKE_SOURCE_DIR}/vk_mxmodel/main.cpp ${CMAKE_SOURCE_DIR}/vk_mxmodel/vk.cpp)
target_include_directories(vk_mxmodel PRIVATE 
    ${CMAKE_SOURCE_DIR}/libmx 
    ${SDL2_INCLUDE_DIRS} 
    ${SDL2_TTF_INCLUDE_DIRS}
)

target_link_libraries(vk_mxmodel PUBLIC SDL2::SDL2main mxvk $<$<NOT:$<BOOL:${MOLTEN}>>:volk>)
