option(BUILD_STATIC_LIB "Build libmx2 as a static library" OFF)

# ─── Base SDL2 library: mx ───────────────────────────────────────────────────
set(MX_BASE_SOURCES
    mx.cpp util.cpp loadpng.cpp font.cpp cfg.cpp texture.cpp
    exception.cpp joystick.cpp jpeg.cpp input.cpp sound.cpp
    tee_stream.cpp
)

if(BUILD_STATIC_LIB)
    add_library(mx STATIC ${MX_BASE_SOURCES})
    message(STATUS "Building mx (base SDL2) as a static library.")
else()
    add_library(mx SHARED ${MX_BASE_SOURCES})
    message(STATUS "Building mx (base SDL2) as a shared library.")
endif()

find_package(SDL2 REQUIRED)

target_include_directories(mx PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${INSTALL_INCLUDE_DIR}>
    ${SDL2_INCLUDE_DIRS}
    ${SDL2_TTF_INCLUDE_DIRS}
    ${IMG_INCLUDE}
    ${MIXER_INCLUDE}
)

add_library(libmx2::mx ALIAS mx)
target_link_libraries(mx PUBLIC SDL2::SDL2 SDL2_ttf::SDL2_ttf ${IMG_LIB} ${MIXER_LIB})

install(TARGETS mx
    EXPORT libmx2Targets
    LIBRARY DESTINATION ${INSTALL_LIB_DIR}
    INCLUDES DESTINATION ${INSTALL_INCLUDE_DIR}
)

install(FILES mx.hpp argz.hpp util.hpp jpeg.hpp loadpng.hpp object.hpp font.hpp cfg.hpp tee_stream.hpp texture.hpp exception.hpp joystick.hpp wrapper.hpp input.hpp sound.hpp
    DESTINATION ${INSTALL_INCLUDE_DIR}
)
install(FILES ${CMAKE_BINARY_DIR}/config.h DESTINATION ${INSTALL_INCLUDE_DIR})

# ─── OpenGL library: mxgl ────────────────────────────────────────────────────
if(OPENGL)
    set(MXGL_SOURCES
        gl.cpp model.cpp console.cpp src/glad.c
    )

    if(BUILD_STATIC_LIB)
        add_library(mxgl STATIC ${MXGL_SOURCES})
        message(STATUS "Building mxgl (OpenGL) as a static library.")
    else()
        add_library(mxgl SHARED ${MXGL_SOURCES})
        message(STATUS "Building mxgl (OpenGL) as a shared library.")
    endif()

    target_include_directories(mxgl PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${INSTALL_INCLUDE_DIR}>
        ${SDL2_INCLUDE_DIRS}
        ${SDL2_TTF_INCLUDE_DIRS}
        ${GL_INCLUDE}
    )

    target_compile_definitions(mxgl PUBLIC WITH_GL)

    add_library(libmx2::mxgl ALIAS mxgl)
    target_link_libraries(mxgl PUBLIC mx ${GL_LIB})

    install(TARGETS mxgl
        EXPORT libmx2Targets
        LIBRARY DESTINATION ${INSTALL_LIB_DIR}
        INCLUDES DESTINATION ${INSTALL_INCLUDE_DIR}
    )

    install(FILES gl.hpp model.hpp console.hpp DESTINATION ${INSTALL_INCLUDE_DIR})
    install(FILES include/glad/glad.h DESTINATION ${INSTALL_INCLUDE_DIR}/glad)
    install(FILES include/KHR/khrplatform.h DESTINATION ${INSTALL_INCLUDE_DIR}/KHR)
endif()

# ─── Vulkan library: mxvk (always static to share volk symbol table) ─────────
if(VULKAN OR MOLTEN)
    add_library(mxvk STATIC vk.cpp vk_model.cpp vk_text.cpp vk_sprite.cpp)
    set_target_properties(mxvk PROPERTIES POSITION_INDEPENDENT_CODE ON)
    message(STATUS "Building mxvk (Vulkan) as a static library.")

    target_include_directories(mxvk PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/volk>
        $<INSTALL_INTERFACE:${INSTALL_INCLUDE_DIR}>
        $<INSTALL_INTERFACE:${INSTALL_INCLUDE_DIR}/volk>
        ${SDL2_INCLUDE_DIRS}
        ${SDL2_TTF_INCLUDE_DIRS}
        ${VK_INCLUDE}
    )

    target_compile_definitions(mxvk PUBLIC WITH_VK)
    if(MOLTEN)
        target_compile_definitions(mxvk PUBLIC WITH_MOLTEN)
    endif()

    target_link_libraries(mxvk PUBLIC mx ${VK_LIB})

    if(MOLTEN)
        target_link_directories(mxvk PUBLIC ${MOLTEN_LINK_DIR})
        target_link_libraries(mxvk PUBLIC ${MOLTEN_FRAMEWORKS})
    else()
        target_link_libraries(mxvk PUBLIC volk)
    endif()

    add_library(libmx2::mxvk ALIAS mxvk)

    install(TARGETS mxvk
        EXPORT libmx2Targets
        LIBRARY DESTINATION ${INSTALL_LIB_DIR}
        INCLUDES DESTINATION ${INSTALL_INCLUDE_DIR}
    )

    install(FILES vk.hpp vk_model.hpp vk_text.hpp vk_sprite.hpp DESTINATION ${INSTALL_INCLUDE_DIR})
    install(FILES include/volk/volk.h DESTINATION ${INSTALL_INCLUDE_DIR}/volk)
endif()

if(OPENGL AND CV)

find_package(OpenCV REQUIRED)
add_library(mxgl_cv SHARED gl_cv.cpp)
add_library(libmx2::mxgl_cv ALIAS mxgl_cv)

target_include_directories(mxgl_cv PRIVATE
    ${OpenCV_INCLUDE_DIRS}
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${INSTALL_INCLUDE_DIR}>
    ${SDL2_INCLUDE_DIRS}
    ${SDL2_TTF_INCLUDE_DIRS}
    ${GL_INCLUDE}
    )
    target_compile_definitions(mxgl_cv PUBLIC WITH_GL)
    target_link_libraries(mxgl_cv ${OpenCV_LIBS})
    install(TARGETS mxgl_cv
        EXPORT libmx2Targets
        LIBRARY DESTINATION ${INSTALL_LIB_DIR}
        INCLUDES DESTINATION ${INSTALL_INCLUDE_DIR}
    )
    message(STATUS "Installing gl_cv.hpp to: ${CMAKE_INSTALL_PREFIX}/${INSTALL_INCLUDE_DIR}")    
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/gl_cv.hpp")
            message(FATAL_ERROR "Could not find gl_cv.hpp in ${CMAKE_CURRENT_SOURCE_DIR}")
    endif()
    install(FILES gl_cv.hpp DESTINATION ${INSTALL_INCLUDE_DIR})
endif()

if(VULKAN AND CV)

find_package(OpenCV REQUIRED)
add_library(mxvk_cv SHARED vk_cv.cpp)
add_library(libmx2::mxvk_cv ALIAS mxvk_cv)

target_include_directories(mxvk_cv PRIVATE
    ${OpenCV_INCLUDE_DIRS}
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/volk>
    $<INSTALL_INTERFACE:${INSTALL_INCLUDE_DIR}>
    $<INSTALL_INTERFACE:${INSTALL_INCLUDE_DIR}/volk>
    ${SDL2_INCLUDE_DIRS}
    ${SDL2_TTF_INCLUDE_DIRS}
    ${VK_INCLUDE}
)
target_link_libraries(mxvk_cv ${OpenCV_LIBS})
install(TARGETS mxvk_cv
    EXPORT libmx2Targets
    LIBRARY DESTINATION ${INSTALL_LIB_DIR}
    INCLUDES DESTINATION ${INSTALL_INCLUDE_DIR}
)
message(STATUS "Installing vk_cv.hpp to: ${CMAKE_INSTALL_PREFIX}/${INSTALL_INCLUDE_DIR}")    
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vk_cv.hpp")
        message(FATAL_ERROR "Could not find vk_cv.hpp in ${CMAKE_CURRENT_SOURCE_DIR}")
endif()
install(FILES vk_cv.hpp DESTINATION ${INSTALL_INCLUDE_DIR})
endif()


# ─── Export / Install ─────────────────────────────────────────────────────────
install(EXPORT libmx2Targets
    FILE libmx2Targets.cmake
    NAMESPACE libmx2::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libmx2
)
