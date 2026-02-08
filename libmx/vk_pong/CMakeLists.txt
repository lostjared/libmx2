add_executable(vk_pong_game ${CMAKE_SOURCE_DIR}/vk_pong/pong.cpp ${CMAKE_SOURCE_DIR}/vk_pong/vk.cpp)
target_include_directories(vk_pong_game PRIVATE 
    ${CMAKE_SOURCE_DIR}/libmx 
    ${SDL2_INCLUDE_DIRS} 
    ${SDL2_TTF_INCLUDE_DIRS}
)

target_link_libraries(vk_pong_game PUBLIC SDL2::SDL2main mxvk volk)

