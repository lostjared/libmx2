
if(VULKAN OR MOLTEN)
    add_executable(vk_cross_stars mxcross.cpp skeleton.cpp vk.cpp)
    target_include_directories(vk_cross_stars PRIVATE 
        ${CMAKE_SOURCE_DIR}/libmx 
        ${CMAKE_SOURCE_DIR}/volk
        ${SDL2_INCLUDE_DIRS} 
        ${SDL2_TTF_INCLUDE_DIRS}
        ${VK_INCLUDE}
    )
else()
    add_executable(vk_cross_stars mxcross.cpp skeleton.cpp)
    target_include_directories(vk_cross_stars PRIVATE 
        ${CMAKE_SOURCE_DIR}/libmx 
        ${SDL2_INCLUDE_DIRS} 
        ${SDL2_TTF_INCLUDE_DIRS}
    )
endif()

if(MOLTEN)
    target_link_directories(vk_cross_stars PRIVATE ${MOLTEN_LINK_DIR})
    target_link_libraries(vk_cross_stars PRIVATE ${MOLTEN_FRAMEWORKS})
    target_link_libraries(vk_cross_stars PUBLIC SDL2::SDL2 SDL2::SDL2main SDL2_ttf::SDL2_ttf mx ${IMG_LIB} ${GL_LIB} ${MIXER_LIB} ${VK_LIB})
elseif(VULKAN)
    target_link_libraries(vk_cross_stars PUBLIC SDL2::SDL2 SDL2::SDL2main SDL2_ttf::SDL2_ttf mx volk ${IMG_LIB} ${GL_LIB} ${MIXER_LIB} ${VK_LIB})
else()
    target_link_libraries(vk_cross_stars PUBLIC SDL2::SDL2 SDL2::SDL2main SDL2_ttf::SDL2_ttf mx ${IMG_LIB} ${GL_LIB} ${MIXER_LIB})
endif()

