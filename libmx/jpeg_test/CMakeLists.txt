add_executable(jpeg_test main.cpp)
target_include_directories(jpeg_test PRIVATE 
    ${CMAKE_SOURCE_DIR}/libmx 
    ${SDL2_INCLUDE_DIRS} 
    ${SDL2_TTF_INCLUDE_DIRS}
)
target_link_libraries(jpeg_test PRIVATE mx ${SDL2_LIBRARIES} ${SDL2_TTF_LIBRARIES})
