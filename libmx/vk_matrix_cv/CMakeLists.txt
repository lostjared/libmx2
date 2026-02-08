find_package(OpenCV REQUIRED)
add_executable(vk_matrix_cv ${CMAKE_SOURCE_DIR}/vk_matrix_cv/skeleton.cpp ${CMAKE_SOURCE_DIR}/vk_matrix_cv/vk.cpp)
target_include_directories(vk_matrix_cv PRIVATE 
    ${CMAKE_SOURCE_DIR}/libmx 
    ${SDL2_INCLUDE_DIRS} 
    ${SDL2_TTF_INCLUDE_DIRS}
    ${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(vk_matrix_cv PUBLIC SDL2::SDL2main mxvk volk ${OpenCV_LIBS})

