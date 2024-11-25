add_executable(mad mad.cpp)
target_include_directories(mad PRIVATE 
    ${CMAKE_SOURCE_DIR}/libmx 
    ${SDL2_INCLUDE_DIRS} 
    ${SDL2_TTF_INCLUDE_DIRS}
)
target_link_libraries(mad PRIVATE mx ${SDL2_LIBRARIES} ${SDL2_TTF_LIBRARIES})
