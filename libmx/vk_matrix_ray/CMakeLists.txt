add_executable(vk_matrix_ray ${CMAKE_SOURCE_DIR}/vk_matrix_ray/skeleton.cpp ${CMAKE_SOURCE_DIR}/vk_matrix_ray/vk.cpp)
target_include_directories(vk_matrix_ray PRIVATE 
    ${CMAKE_SOURCE_DIR}/libmx 
    ${CMAKE_SOURCE_DIR}/volk
    ${SDL2_INCLUDE_DIRS} 
    ${SDL2_TTF_INCLUDE_DIRS}
    ${VK_INCLUDE}
)

if(MOLTEN)
link_directories(${MOLTEN_PATH}/lib)
target_link_libraries(vk_matrix_ray PRIVATE
    "-framework QuartzCore"
    "-framework Metal"
    "-framework IOSurface"
    "-framework CoreGraphics")
target_link_libraries(vk_matrix_ray PUBLIC SDL2::SDL2 SDL2::SDL2main SDL2_ttf::SDL2_ttf mx ${IMG_LIB} ${GL_LIB} ${MIXER_LIB} ${VK_LIB})
else()
target_link_libraries(vk_matrix_ray PUBLIC SDL2::SDL2 SDL2::SDL2main SDL2_ttf::SDL2_ttf mx volk ${IMG_LIB} ${GL_LIB} ${MIXER_LIB} ${VK_LIB})
endif()

