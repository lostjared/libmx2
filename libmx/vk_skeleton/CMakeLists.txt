add_executable(vk_skeleton ${CMAKE_SOURCE_DIR}/vk_skeleton/skeleton.cpp ${CMAKE_SOURCE_DIR}/vk_skeleton/vk.cpp)
target_include_directories(vk_skeleton PRIVATE 
    ${CMAKE_SOURCE_DIR}/libmx 
    ${SDL2_INCLUDE_DIRS} 
    ${SDL2_TTF_INCLUDE_DIRS}
)

target_link_libraries(vk_skeleton PUBLIC SDL2::SDL2main mxvk volk)

