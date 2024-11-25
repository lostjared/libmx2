add_executable(frogger frogger.cpp)
target_include_directories(frogger PRIVATE 
    ${CMAKE_SOURCE_DIR}/libmx 
    ${SDL2_INCLUDE_DIRS} 
    ${SDL2_TTF_INCLUDE_DIRS}
)
target_link_libraries(frogger PRIVATE mx ${SDL2_LIBRARIES} ${SDL2_TTF_LIBRARIES})
