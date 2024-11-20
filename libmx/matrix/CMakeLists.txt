add_executable(matrix matrix.cpp)
target_include_directories(matrix PRIVATE 
    ${CMAKE_SOURCE_DIR}/libmx 
    ${SDL2_INCLUDE_DIRS} 
    ${SDL2_TTF_INCLUDE_DIRS}
)
target_link_libraries(matrix PRIVATE mx ${SDL2_LIBRARIES} ${SDL2_TTF_LIBRARIES})
