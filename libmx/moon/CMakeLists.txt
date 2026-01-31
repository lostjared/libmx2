
if(VULKAN)
add_executable(moon mxcross.cpp skeleton.cpp vk.cpp)
target_include_directories(moon PRIVATE 
    ${CMAKE_SOURCE_DIR}/libmx 
    ${SDL2_INCLUDE_DIRS} 
    ${SDL2_TTF_INCLUDE_DIRS}
    ${VK_INCLUDE}
)
else()
add_executable(moon mxcross.cpp skeleton.cpp)
target_include_directories(moon PRIVATE 
    ${CMAKE_SOURCE_DIR}/libmx 
    ${SDL2_INCLUDE_DIRS} 
    ${SDL2_TTF_INCLUDE_DIRS}
)
endif()
if(MOLTEN)
link_directories(${MOLTEN_PATH}/lib)
target_link_libraries(cross PRIVATE
    "-framework QuartzCore"
    "-framework Metal"
    "-framework IOSurface"
    "-framework CoreGraphics")
target_link_libraries(moon PUBLIC SDL2::SDL2 SDL2::SDL2main SDL2_ttf::SDL2_ttf mx ${IMG_LIB} ${GL_LIB} ${MIXER_LIB} ${VK_LIB})
else()
if(VULKAN)
target_link_libraries(moon PUBLIC SDL2::SDL2 SDL2::SDL2main SDL2_ttf::SDL2_ttf mx volk ${IMG_LIB} ${GL_LIB} ${MIXER_LIB} ${VK_LIB})
else()
target_link_libraries(moon PUBLIC SDL2::SDL2 SDL2::SDL2main SDL2_ttf::SDL2_ttf mx ${IMG_LIB} ${GL_LIB} ${MIXER_LIB})
endif()
endif()