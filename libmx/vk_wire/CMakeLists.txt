if(MOLTEN)
add_executable(vk_wire ${CMAKE_SOURCE_DIR}/vk_wire/main.cpp ${CMAKE_SOURCE_DIR}/vk_wire/vk.cpp)
else()
add_executable(vk_wire ${CMAKE_SOURCE_DIR}/vk_wire/main.cpp ${CMAKE_SOURCE_DIR}/vk_wire/vk.cpp ${CMAKE_SOURCE_DIR}/vk_wire/volk.c)
endif()
target_include_directories(vk_wire PRIVATE 
    ${CMAKE_SOURCE_DIR}/libmx 
    ${SDL2_INCLUDE_DIRS} 
    ${SDL2_TTF_INCLUDE_DIRS}y
    ${VK_INCLUDE}
)

if(MOLTEN)
link_directories(${MOLTEN_PATH}/lib)
target_link_libraries(vk_wire PRIVATE
    "-framework QuartzCore"
    "-framework Metal"
    "-framework IOSurface"
    "-framework CoreGraphics")

target_link_libraries(vk_wire PUBLIC SDL2::SDL2 SDL2::SDL2main SDL2_ttf::SDL2_ttf mx ${IMG_LIB} ${GL_LIB} ${MIXER_LIB} ${VK_LIB})
else()
target_link_libraries(vk_wire PUBLIC SDL2::SDL2 SDL2::SDL2main SDL2_ttf::SDL2_ttf mx ${IMG_LIB} ${GL_LIB} ${MIXER_LIB} ${VK_LIB})
endif()

