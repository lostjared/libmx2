
if(VULKAN OR MOLTEN)
    add_executable(vk_cross_stars_frag mxcross.cpp skeleton.cpp vk.cpp)
    target_include_directories(vk_cross_stars_frag PRIVATE 
        ${CMAKE_SOURCE_DIR}/libmx 
        ${SDL2_INCLUDE_DIRS} 
        ${SDL2_TTF_INCLUDE_DIRS}
    )
    target_link_libraries(vk_cross_stars_frag PUBLIC SDL2::SDL2main mxvk $<$<NOT:$<BOOL:${MOLTEN}>>:volk>)
else()
    add_executable(vk_cross_stars_frag mxcross.cpp skeleton.cpp)
    target_include_directories(vk_cross_stars_frag PRIVATE 
        ${CMAKE_SOURCE_DIR}/libmx 
        ${SDL2_INCLUDE_DIRS} 
        ${SDL2_TTF_INCLUDE_DIRS}
    )
    target_link_libraries(vk_cross_stars_frag PUBLIC SDL2::SDL2main mxgl)
endif()

