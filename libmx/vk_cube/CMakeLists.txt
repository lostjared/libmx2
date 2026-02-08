add_executable(vk_cube main.cpp ${CMAKE_SOURCE_DIR}/vk_cube/vk.cpp)
target_include_directories(vk_cube PRIVATE 
    ${CMAKE_SOURCE_DIR}/libmx 
    ${SDL2_INCLUDE_DIRS} 
    ${SDL2_TTF_INCLUDE_DIRS}
)

target_link_libraries(vk_cube PUBLIC SDL2::SDL2main mxvk volk)
