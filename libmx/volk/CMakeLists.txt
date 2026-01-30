
add_library(volk STATIC volk.c)

target_include_directories(volk PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${INSTALL_INCLUDE_DIR}/volk>
    ${VK_INCLUDE}
)

if(NOT WIN32)
    target_link_libraries(volk PUBLIC ${CMAKE_DL_LIBS})
endif()

add_library(libmx2::volk ALIAS volk)
