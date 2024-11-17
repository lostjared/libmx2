add_library(mx SHARED mx.cpp util.cpp loadpng.cpp font.cpp)
find_package(SDL2 REQUIRED)
find_package(SDL2_ttf REQUIRED)
target_include_directories(mx PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${INSTALL_INCLUDE_DIR}>
    ${SDL2_INCLUDE_DIRS}
    ${SDL2_TTF_INCLUDE_DIRS}
    ${PNG_DIRS}
)
target_link_libraries(mx PUBLIC SDL2::SDL2 SDL2_ttf::SDL2_ttf ${PNG_LIBS})
add_library(libmx2::mx ALIAS mx)
install(TARGETS mx
    EXPORT libmx2Targets
    LIBRARY DESTINATION ${INSTALL_LIB_DIR}
    INCLUDES DESTINATION ${INSTALL_INCLUDE_DIR}
)
install(FILES mx.hpp argz.hpp util.hpp loadpng.hpp object.hpp font.hpp
    DESTINATION ${INSTALL_INCLUDE_DIR}
)
install(EXPORT libmx2Targets
    FILE libmx2Targets.cmake
    NAMESPACE libmx2::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libmx2
)
