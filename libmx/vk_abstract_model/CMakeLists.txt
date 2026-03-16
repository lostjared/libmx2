add_executable(vk_abstract_model
    ${CMAKE_SOURCE_DIR}/vk_abstract_model/abstract.cpp
)

target_include_directories(vk_abstract_model PRIVATE
    ${CMAKE_SOURCE_DIR}/libmx
    ${SDL2_INCLUDE_DIRS}
    ${SDL2_TTF_INCLUDE_DIRS}
)

target_link_libraries(vk_abstract_model PUBLIC SDL2::SDL2main mxvk $<$<NOT:$<BOOL:${MOLTEN}>>:volk>)
