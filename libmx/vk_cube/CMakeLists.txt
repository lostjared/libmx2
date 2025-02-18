add_executable(vk_cube main.cpp)
target_include_directories(vk_cube PRIVATE 
    ${CMAKE_SOURCE_DIR}/libmx 
    ${SDL2_INCLUDE_DIRS} 
    ${SDL2_TTF_INCLUDE_DIRS}
    ${VK_INCLUDE}
)

target_link_libraries(vk_cube PRIVATE mx)

