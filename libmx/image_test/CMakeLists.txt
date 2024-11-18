add_executable(image_test main.cpp)
target_include_directories(image_test PRIVATE 
    ${CMAKE_SOURCE_DIR}/libmx 
    ${SDL2_INCLUDE_DIRS} 
    ${SDL2_TTF_INCLUDE_DIRS}
)
target_link_libraries(image_test PRIVATE mx ${SDL2_LIBRARIES} ${SDL2_TTF_LIBRARIES})
