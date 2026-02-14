find_package(OpenCV REQUIRED)
add_executable(vk_opencv_moon ${CMAKE_SOURCE_DIR}/vk_opencv_moon/main.cpp)
if(MINGW)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static -static-libgcc -static-libstdc++")
endif()
target_include_directories(vk_opencv_moon PRIVATE 
    ${CMAKE_SOURCE_DIR}/libmx 
    ${SDL2_INCLUDE_DIRS} 
    ${SDL2_TTF_INCLUDE_DIRS}
    ${OpenCV_INCLUDE_DIRS}
)
target_link_libraries(vk_opencv_moon PUBLIC SDL2::SDL2main mxvk $<$<NOT:$<BOOL:${MOLTEN}>>:volk> ${OpenCV_LIBS})

