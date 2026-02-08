
if(VULKAN OR MOLTEN)
add_executable(cross ${CMAKE_SOURCE_DIR}/cross/mxcross.cpp ${CMAKE_SOURCE_DIR}/cross/skeleton.cpp ${CMAKE_SOURCE_DIR}/cross/vk.cpp)
target_include_directories(cross PRIVATE 
    ${CMAKE_SOURCE_DIR}/libmx 
    ${SDL2_INCLUDE_DIRS} 
    ${SDL2_TTF_INCLUDE_DIRS}
)
target_link_libraries(cross PUBLIC SDL2::SDL2main mxgl mxvk volk)
else()
add_executable(cross ${CMAKE_SOURCE_DIR}/cross/mxcross.cpp ${CMAKE_SOURCE_DIR}/cross/skeleton.cpp)
target_include_directories(cross PRIVATE 
    ${CMAKE_SOURCE_DIR}/libmx 
    ${SDL2_INCLUDE_DIRS} 
    ${SDL2_TTF_INCLUDE_DIRS}
)
target_link_libraries(cross PUBLIC SDL2::SDL2main mxgl)
endif()

