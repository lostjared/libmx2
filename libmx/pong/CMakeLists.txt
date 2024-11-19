add_executable(pong pong.cpp)
target_include_directories(pong PRIVATE 
    ${CMAKE_SOURCE_DIR}/libmx 
    ${SDL2_INCLUDE_DIRS} 
    ${SDL2_TTF_INCLUDE_DIRS}
)
target_link_libraries(pong PRIVATE mx ${SDL2_LIBRARIES} ${SDL2_TTF_LIBRARIES})
