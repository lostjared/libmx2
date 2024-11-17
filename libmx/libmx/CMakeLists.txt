add_library(mx SHARED mx.cpp)
target_include_directories(mx PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR} 
    ${SDL2_INCLUDE_DIRS} 
    ${SDL2_TTF_INCLUDE_DIRS}
)
target_link_libraries(mx PUBLIC ${SDL2_LIBRARIES} ${SDL2_TTF_LIBRARIES})