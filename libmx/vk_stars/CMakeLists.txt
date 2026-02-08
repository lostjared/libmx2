add_executable(vk_stars ${CMAKE_SOURCE_DIR}/vk_stars/main.cpp ${CMAKE_SOURCE_DIR}/vk_stars/vk.cpp)
target_include_directories(vk_stars PRIVATE 
    ${CMAKE_SOURCE_DIR}/libmx 
    ${SDL2_INCLUDE_DIRS} 
    ${SDL2_TTF_INCLUDE_DIRS}
)

target_link_libraries(vk_stars PUBLIC SDL2::SDL2main mxvk volk)

