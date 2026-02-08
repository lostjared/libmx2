add_executable(vk_borg ${CMAKE_SOURCE_DIR}/vk_borg/main.cpp ${CMAKE_SOURCE_DIR}/vk_borg/vk.cpp)
target_include_directories(vk_borg PRIVATE 
    ${CMAKE_SOURCE_DIR}/libmx 
    ${SDL2_INCLUDE_DIRS} 
    ${SDL2_TTF_INCLUDE_DIRS}
)

target_link_libraries(vk_borg PUBLIC SDL2::SDL2main mxvk volk)

