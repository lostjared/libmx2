add_executable(breakout breakout.cpp)
target_include_directories(breakout PRIVATE 
    ${CMAKE_SOURCE_DIR}/libmx 
    ${SDL2_INCLUDE_DIRS} 
    ${SDL2_TTF_INCLUDE_DIRS}
)
target_link_libraries(breakout PRIVATE mx ${SDL2_LIBRARIES} ${SDL2_TTF_LIBRARIES})
