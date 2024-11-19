add_executable(blocks blocks.cpp)

if(WIN32)
    set_target_properties(blocks PROPERTIES WIN32_EXECUTABLE ON)
endif()

target_include_directories(blocks PRIVATE 
    ${CMAKE_SOURCE_DIR}/libmx 
    ${SDL2_INCLUDE_DIRS} 
    ${SDL2_TTF_INCLUDE_DIRS}
)
target_link_libraries(blocks PRIVATE mx ${SDL2_LIBRARIES} ${SDL2_TTF_LIBRARIES})
