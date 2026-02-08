
if(VULKAN OR MOLTEN)
add_executable(moon mxcross.cpp skeleton.cpp vk.cpp)
target_include_directories(moon PRIVATE 
    ${CMAKE_SOURCE_DIR}/libmx 
    ${SDL2_INCLUDE_DIRS} 
    ${SDL2_TTF_INCLUDE_DIRS}
)
target_link_libraries(moon PUBLIC SDL2::SDL2main mxgl mxvk volk)
else()
add_executable(moon mxcross.cpp skeleton.cpp)
target_include_directories(moon PRIVATE 
    ${CMAKE_SOURCE_DIR}/libmx 
    ${SDL2_INCLUDE_DIRS} 
    ${SDL2_TTF_INCLUDE_DIRS}
)
target_link_libraries(moon PUBLIC SDL2::SDL2main mxgl)
endif()