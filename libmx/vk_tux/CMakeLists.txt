add_executable(vk_tux
    ${CMAKE_SOURCE_DIR}/vk_tux/tux.cpp
)

target_include_directories(vk_tux PRIVATE
    ${CMAKE_SOURCE_DIR}/libmx
    ${SDL2_INCLUDE_DIRS}
    ${SDL2_TTF_INCLUDE_DIRS}
)

target_link_libraries(vk_tux PUBLIC SDL2::SDL2main mxvk $<$<NOT:$<BOOL:${MOLTEN}>>:volk>)
