add_executable(asteroids asteroids.cpp game.cpp)

if(WIN32)
    set_target_properties(asteroids PROPERTIES WIN32_EXECUTABLE ON)
endif()

target_include_directories(asteroids PRIVATE 
    ${CMAKE_SOURCE_DIR}/libmx 
    ${SDL2_INCLUDE_DIRS} 
    ${SDL2_TTF_INCLUDE_DIRS}
)
target_link_libraries(asteroids PRIVATE mx ${SDL2_LIBRARIES} ${SDL2_TTF_LIBRARIES})
