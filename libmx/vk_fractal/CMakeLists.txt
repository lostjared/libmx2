if(MOLTEN)
add_executable(vk_fractal ${CMAKE_SOURCE_DIR}/vk_fractal/fractal.cpp ${CMAKE_SOURCE_DIR}/vk_fractal/vk.cpp)
else()
add_executable(vk_fractal ${CMAKE_SOURCE_DIR}/vk_fractal/fractal.cpp ${CMAKE_SOURCE_DIR}/vk_fractal/vk.cpp ${CMAKE_SOURCE_DIR}/vk_fractal/volk.c)
endif()
target_include_directories(vk_fractal PRIVATE 
    ${CMAKE_SOURCE_DIR}/libmx 
    ${SDL2_INCLUDE_DIRS} 
    ${SDL2_TTF_INCLUDE_DIRS}
    ${VK_INCLUDE}
)

if(MOLTEN)
link_directories(${MOLTEN_PATH}/lib)
target_link_libraries(vk_pong PRIVATE
    "-framework QuartzCore"
    "-framework Metal"
    "-framework IOSurface"
    "-framework CoreGraphics")
target_link_libraries(vk_fractal PRIVATE
    "-framework QuartzCore"
    "-framework Metal"
    "-framework IOSurface"
    "-framework CoreGraphics")

target_link_libraries(vk_fractal PUBLIC SDL2::SDL2 SDL2::SDL2main SDL2_ttf::SDL2_ttf mx ${IMG_LIB} ${GL_LIB} ${MIXER_LIB} ${VK_LIB})
else()
target_link_libraries(vk_fractal PUBLIC SDL2::SDL2 SDL2::SDL2main SDL2_ttf::SDL2_ttf mx ${IMG_LIB} ${GL_LIB} ${MIXER_LIB} ${VK_LIB})
endif()

