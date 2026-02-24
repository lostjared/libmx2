add_executable(gl_cv_skeleton skeleton.cpp)
target_include_directories(gl_cv_skeleton PRIVATE 
    ${CMAKE_SOURCE_DIR}/libmx 
    ${SDL2_INCLUDE_DIRS} 
    ${SDL2_TTF_INCLUDE_DIRS}
)
target_link_libraries(gl_cv_skeleton PRIVATE mxgl ${SDL2_LIBRARIES} ${SDL2_TTF_LIBRARIES} mxgl_cv ${OpenCV_LIBS})
