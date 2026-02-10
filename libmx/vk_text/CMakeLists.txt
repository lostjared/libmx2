add_executable(vk_text ${CMAKE_SOURCE_DIR}/vk_text/main.cpp ${CMAKE_SOURCE_DIR}/vk_text/vk.cpp)
target_include_directories(vk_text PRIVATE 
    ${CMAKE_SOURCE_DIR}/libmx 
    ${SDL2_INCLUDE_DIRS} 
    ${SDL2_TTF_INCLUDE_DIRS}
)

target_link_libraries(vk_text PUBLIC SDL2::SDL2main mxvk $<$<NOT:$<BOOL:${MOLTEN}>>:volk>)

