add_executable(vk_stickball
    ${CMAKE_SOURCE_DIR}/vk_stickball/main.cpp
)

target_include_directories(vk_stickball PRIVATE
    ${CMAKE_SOURCE_DIR}/libmx
    ${SDL2_INCLUDE_DIRS}
    ${SDL2_TTF_INCLUDE_DIRS}
)

target_link_libraries(vk_stickball PUBLIC SDL2::SDL2main mxvk $<$<NOT:$<BOOL:${MOLTEN}>>:volk>)
