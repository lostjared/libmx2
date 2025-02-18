add_executable(vk_ex1 main.cpp)
target_include_directories(vk_ex1 PRIVATE 
    ${CMAKE_SOURCE_DIR}/libmx 
    ${SDL2_INCLUDE_DIRS} 
    ${SDL2_TTF_INCLUDE_DIRS}
    ${VK_INCLUDE}
)
target_link_libraries(vk_ex1 PUBLIC mx)


