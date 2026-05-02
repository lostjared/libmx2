add_executable(vk_compute
    ${CMAKE_SOURCE_DIR}/vk_compute/main.cpp
)

target_include_directories(vk_compute PRIVATE
    ${CMAKE_SOURCE_DIR}/libmx
    ${SDL2_INCLUDE_DIRS}
    ${SDL2_TTF_INCLUDE_DIRS}
    ${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(vk_compute PUBLIC
    SDL2::SDL2main
    mxvk
    mxvk_cv
    ${OpenCV_LIBS}
    $<$<NOT:$<BOOL:${MOLTEN}>>:volk>
)

# ── compile compute shader to SPIR-V ─────────────────────────────────────────
find_program(GLSLC_EXECUTABLE NAMES glslc)
if(GLSLC_EXECUTABLE)
    set(_COMP_SRC ${CMAKE_SOURCE_DIR}/vk_compute/data/compute.comp)
    set(_COMP_SPV ${CMAKE_SOURCE_DIR}/vk_compute/data/compute.spv)
    add_custom_command(
        OUTPUT  ${_COMP_SPV}
        COMMAND ${GLSLC_EXECUTABLE} -fshader-stage=compute ${_COMP_SRC} -o ${_COMP_SPV}
        DEPENDS ${_COMP_SRC}
        COMMENT "Compiling vk_compute/data/compute.comp → compute.spv"
        VERBATIM
    )
    add_custom_target(vk_compute_shaders ALL DEPENDS ${_COMP_SPV})
    add_dependencies(vk_compute vk_compute_shaders)
else()
    message(WARNING "glslc not found – compute.spv must be compiled manually before running vk_compute")
endif()

