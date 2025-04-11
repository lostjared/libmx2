add_executable(gl_asteroids asteroids.cpp console.cpp)
target_include_directories(gl_asteroids PRIVATE 
    ${CMAKE_SOURCE_DIR}/libmx 
    ${SDL2_INCLUDE_DIRS} 
    ${SDL2_TTF_INCLUDE_DIRS}
)
target_link_libraries(gl_asteroids PRIVATE mx ${SDL2_LIBRARIES} ${SDL2_TTF_LIBRARIES})
if(MINGW)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -mwindows")
endif()
