add_executable(vk_image ${CMAKE_SOURCE_DIR}/vk_image/main.cpp ${CMAKE_SOURCE_DIR}/vk_image/vk.cpp)
target_include_directories(vk_image PRIVATE 
    ${CMAKE_SOURCE_DIR}/libmx 
    ${SDL2_INCLUDE_DIRS} 
    ${SDL2_TTF_INCLUDE_DIRS}
)

target_link_libraries(vk_image PUBLIC SDL2::SDL2main mxvk volk)

