add_executable(vk_spacerox ${CMAKE_SOURCE_DIR}/vk_spacerox/space.cpp ${CMAKE_SOURCE_DIR}/vk_spacerox/vk.cpp)
target_include_directories(vk_spacerox PRIVATE 
    ${CMAKE_SOURCE_DIR}/libmx 
    ${SDL2_INCLUDE_DIRS} 
    ${SDL2_TTF_INCLUDE_DIRS}
)

target_link_libraries(vk_spacerox PUBLIC SDL2::SDL2main mxvk volk)
