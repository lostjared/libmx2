add_executable(input_test main.cpp)
target_include_directories(input_test PRIVATE 
    ${CMAKE_SOURCE_DIR}/libmx 
    ${SDL2_INCLUDE_DIRS} 
    ${SDL2_TTF_INCLUDE_DIRS}
)
target_link_libraries(input_test PRIVATE mx ${SDL2_LIBRARIES} ${SDL2_TTF_LIBRARIES})
