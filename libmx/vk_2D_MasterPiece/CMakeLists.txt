add_executable(vk_2D_MasterPiece ${CMAKE_SOURCE_DIR}/vk_2D_MasterPiece/skeleton.cpp ${CMAKE_SOURCE_DIR}/vk_2D_MasterPiece/vk.cpp)
target_include_directories(vk_2D_MasterPiece PRIVATE 
    ${CMAKE_SOURCE_DIR}/libmx 
    ${SDL2_INCLUDE_DIRS} 
    ${SDL2_TTF_INCLUDE_DIRS}
)

target_link_libraries(vk_2D_MasterPiece PUBLIC SDL2::SDL2main mxvk $<$<NOT:$<BOOL:${MOLTEN}>>:volk>)

