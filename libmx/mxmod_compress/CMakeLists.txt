add_executable(mxmod_compress cmp.cpp)
target_include_directories(mxmod_compress PRIVATE 
    ${CMAKE_SOURCE_DIR}/libmx 
    ${ZLIB_INCLUDE_DIRS}
)
target_link_libraries(mxmod_compress PRIVATE ${ZLIB_LIBRARIES})

install(TARGETS mxmod_compress
        RUNTIME DESTINATION bin)
