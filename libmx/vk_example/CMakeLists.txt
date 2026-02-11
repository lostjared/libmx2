add_executable(vk_example
    ${CMAKE_SOURCE_DIR}/vk_example/main.cpp
)

target_include_directories(vk_example PRIVATE
    ${CMAKE_SOURCE_DIR}/libmx
    ${SDL2_INCLUDE_DIRS}
    ${SDL2_TTF_INCLUDE_DIRS}
)

target_link_libraries(vk_example PUBLIC SDL2::SDL2main mxvk $<$<NOT:$<BOOL:${MOLTEN}>>:volk>)
