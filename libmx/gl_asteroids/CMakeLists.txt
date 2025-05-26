add_executable(gl_asteroids asteroids.cpp)
target_include_directories(gl_asteroids PRIVATE 
    ${CMAKE_SOURCE_DIR}/libmx 
    ${SDL2_INCLUDE_DIRS} 
    ${SDL2_TTF_INCLUDE_DIRS}
)
target_link_libraries(gl_asteroids PRIVATE mx ${SDL2_LIBRARIES} ${SDL2_TTF_LIBRARIES})
if(MINGW)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -mwindows")
endif()
if(WIN32)
  set(RESOURCE_FILE "${CMAKE_CURRENT_SOURCE_DIR}/data/icon.rc")
  set_source_files_properties(
    ${RESOURCE_FILE}
    PROPERTIES
      LANGUAGE RC
  target_sources(gl_asteroids PRIVATE ${RESOURCE_FILE})
endif()