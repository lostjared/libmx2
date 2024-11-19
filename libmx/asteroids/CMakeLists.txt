add_executable(asteroids asteroids.cpp game.cpp)
target_include_directories(asteroids PRIVATE 
    ${CMAKE_SOURCE_DIR}/libmx 
    ${SDL2_INCLUDE_DIRS} 
    ${SDL2_TTF_INCLUDE_DIRS}
)
target_link_libraries(asteroids PRIVATE mx ${SDL2_LIBRARIES} ${SDL2_TTF_LIBRARIES})
