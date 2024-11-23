add_executable(breakout breakout.cpp)
target_include_directories(breakout PRIVATE 
    ${CMAKE_SOURCE_DIR}/libmx 
    ${SDL2_INCLUDE_DIRS} 
    ${SDL2_TTF_INCLUDE_DIRS}
)

if (WIN32)
    set_target_properties(breakout PROPERTIES WIN32_EXECUTABLE ON)
endif()


target_link_libraries(breakout PRIVATE mx ${SDL2_LIBRARIES} ${SDL2_TTF_LIBRARIES})
