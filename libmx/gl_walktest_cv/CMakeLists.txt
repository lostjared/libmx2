add_executable(gl_walktest_cv room.cpp)
target_include_directories(gl_walktest_cv PRIVATE 
    ${CMAKE_SOURCE_DIR}/libmx 
    ${SDL2_INCLUDE_DIRS} 
    ${SDL2_TTF_INCLUDE_DIRS}
)
target_link_libraries(gl_walktest_cv PRIVATE mxgl mxgl_cv ${SDL2_LIBRARIES} ${SDL2_TTF_LIBRARIES} ${OpenCV_LIBS})
