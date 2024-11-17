add_executable(test main.cpp)
target_include_directories(test PRIVATE 
    ${CMAKE_SOURCE_DIR}/libmx 
    ${SDL2_INCLUDE_DIRS} 
    ${SDL2_TTF_INCLUDE_DIRS}
)
target_link_libraries(test PRIVATE mx ${SDL2_LIBRARIES} ${SDL2_TTF_LIBRARIES})
