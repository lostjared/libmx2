add_executable(controller_test main.cpp)
target_include_directories(controller_test PRIVATE 
    ${CMAKE_SOURCE_DIR}/libmx 
    ${SDL2_INCLUDE_DIRS} 
    ${SDL2_TTF_INCLUDE_DIRS}
)
target_link_libraries(controller_test PRIVATE mx ${SDL2_LIBRARIES} ${SDL2_TTF_LIBRARIES})
