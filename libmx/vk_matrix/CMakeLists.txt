add_executable(vk_matrix ${CMAKE_SOURCE_DIR}/vk_matrix/matrix.cpp ${CMAKE_SOURCE_DIR}/vk_matrix/vk.cpp)
target_include_directories(vk_matrix PRIVATE 
    ${CMAKE_SOURCE_DIR}/libmx 
    ${SDL2_INCLUDE_DIRS} 
    ${SDL2_TTF_INCLUDE_DIRS}
)

target_link_libraries(vk_matrix PUBLIC SDL2::SDL2main mxvk volk)

