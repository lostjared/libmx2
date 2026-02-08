add_executable(vk_data ${CMAKE_SOURCE_DIR}/vk_data/main.cpp ${CMAKE_SOURCE_DIR}/vk_data/vk.cpp)
target_include_directories(vk_data PRIVATE 
    ${CMAKE_SOURCE_DIR}/libmx 
    ${SDL2_INCLUDE_DIRS} 
    ${SDL2_TTF_INCLUDE_DIRS}
)

target_link_libraries(vk_data PUBLIC SDL2::SDL2main mxvk volk)

