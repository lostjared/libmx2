add_executable(vk_frag ${CMAKE_SOURCE_DIR}/vk_frag/main.cpp ${CMAKE_SOURCE_DIR}/vk_frag/vk.cpp)
target_include_directories(vk_frag PRIVATE 
    ${CMAKE_SOURCE_DIR}/libmx 
    ${SDL2_INCLUDE_DIRS} 
    ${SDL2_TTF_INCLUDE_DIRS}
)

target_link_libraries(vk_frag PUBLIC SDL2::SDL2main mxvk volk)

