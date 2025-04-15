option(BUILD_STATIC_LIB "Build libmx2 as a static library" OFF)

if(BUILD_STATIC_LIB)
    add_library(mx STATIC mx.cpp util.cpp loadpng.cpp font.cpp cfg.cpp texture.cpp exception.cpp joystick.cpp jpeg.cpp gl.cpp input.cpp sound.cpp model.cpp tee_stream.cpp ${GL_SRC})
    message(STATUS "Building libmx2 as a static library.")
else()
    add_library(mx SHARED mx.cpp util.cpp loadpng.cpp font.cpp cfg.cpp texture.cpp exception.cpp joystick.cpp jpeg.cpp gl.cpp input.cpp sound.cpp model.cpp tee_stream.cpp ${GL_SRC})
    message(STATUS "Building libmx2 as a shared library.")
endif()

find_package(SDL2 REQUIRED)
find_package(SDL2_ttf REQUIRED)


target_include_directories(mx PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/volk>
    $<INSTALL_INTERFACE:${INSTALL_INCLUDE_DIR}>
    ${SDL2_INCLUDE_DIRS}
    ${SDL2_TTF_INCLUDE_DIRS}
    ${IMG_INCLUDE}
    ${GL_INCLUDE}
    ${MIXER_INCLUDE}
    ${VK_INCLUDE}
)

add_library(libmx2::mx ALIAS mx)
install(TARGETS mx
    EXPORT libmx2Targets
    LIBRARY DESTINATION ${INSTALL_LIB_DIR}
    INCLUDES DESTINATION ${INSTALL_INCLUDE_DIR}
)
target_link_libraries(mx PUBLIC SDL2::SDL2 SDL2_ttf::SDL2_ttf ${IMG_LIB} ${GL_LIB} ${MIXER_LIB})

install(FILES mx.hpp gl.hpp vk.hpp argz.hpp util.hpp jpeg.hpp loadpng.hpp object.hpp font.hpp cfg.hpp tee_stream.hpp texture.hpp exception.hpp joystick.hpp wrapper.hpp input.hpp sound.hpp model.hpp console.hpp
    DESTINATION ${INSTALL_INCLUDE_DIR}
)
install(FILES include/glad/glad.h DESTINATION ${INSTALL_INCLUDE_DIR}/glad)
install(FILES include/KHR/khrplatform.h DESTINATION ${INSTALL_INCLUDE_DIR}/KHR)
install(FILES ${CMAKE_BINARY_DIR}/config.h DESTINATION ${INSTALL_INCLUDE_DIR})
install(EXPORT libmx2Targets
    FILE libmx2Targets.cmake
    NAMESPACE libmx2::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libmx2
)
