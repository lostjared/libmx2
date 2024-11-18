add_executable(knights_tour knight.cpp)
target_include_directories(knights_tour PRIVATE 
    ${CMAKE_SOURCE_DIR}/libmx 
    ${SDL2_INCLUDE_DIRS} 
    ${SDL2_TTF_INCLUDE_DIRS}
)
target_link_libraries(knights_tour PRIVATE mx ${SDL2_LIBRARIES} ${SDL2_TTF_LIBRARIES})
