add_executable(vk_spacerox ${CMAKE_SOURCE_DIR}/vk_spacerox/space.cpp ${CMAKE_SOURCE_DIR}/vk_spacerox/vk.cpp)
target_include_directories(vk_spacerox PRIVATE 
    ${CMAKE_SOURCE_DIR}/libmx 
    ${CMAKE_SOURCE_DIR}/volk
    ${SDL2_INCLUDE_DIRS} 
    ${SDL2_TTF_INCLUDE_DIRS}
    ${VK_INCLUDE}
)

if(MOLTEN)
    target_link_directories(vk_spacerox PRIVATE ${MOLTEN_LINK_DIR})
    target_link_libraries(vk_spacerox PRIVATE ${MOLTEN_FRAMEWORKS})
    target_link_libraries(vk_spacerox PUBLIC SDL2::SDL2 SDL2::SDL2main SDL2_ttf::SDL2_ttf mx ${IMG_LIB} ${GL_LIB} ${MIXER_LIB} ${VK_LIB})
else()
    target_link_libraries(vk_spacerox PUBLIC SDL2::SDL2 SDL2::SDL2main SDL2_ttf::SDL2_ttf mx volk ${IMG_LIB} ${GL_LIB} ${MIXER_LIB} ${VK_LIB})
endif()
