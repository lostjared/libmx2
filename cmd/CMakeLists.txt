cmake_minimum_required(VERSION 3.10)
project(mxcmd_cxx VERSION 1.07 LANGUAGES CXX C)


option(CMD "Build command line interface" OFF)
option(CONSOLE "Build graphical console" OFF)
option(SHARED "Build shared version of library" OFF)
option(RELEASE "Build with Optimization" OFF)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(RELEASE)
    set(CMAKE_BUILD_TYPE Release)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3")
else()
    set(CMAKE_BUILD_TYPE Debug)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")
endif()

if(SHARED)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

add_library(mxcmd SHARED
    src/scanner.cpp
    src/string_buffer.cpp
    src/types.cpp
    src/command.cpp
    src/ast.cpp
    src/html.cpp
    src/command_reg.cpp
)

else()
 
add_library(mxcmd STATIC
    src/scanner.cpp
    src/string_buffer.cpp
    src/types.cpp
    src/command.cpp
    src/ast.cpp
    src/html.cpp
    src/command_reg.cpp
)

endif()

list(APPEND header 
    ${CMAKE_SOURCE_DIR}/include/command.hpp 
    ${CMAKE_SOURCE_DIR}/include/scanner.hpp 
    ${CMAKE_SOURCE_DIR}/include/types.hpp 
    ${CMAKE_SOURCE_DIR}/include/string_buffer.hpp
    ${CMAKE_SOURCE_DIR}/include/ast.hpp
    ${CMAKE_SOURCE_DIR}/include/parser.hpp
    ${CMAKE_SOURCE_DIR}/include/game_state.hpp
    ${CMAKE_SOURCE_DIR}/include/html.hpp 
    ${CMAKE_SOURCE_DIR}/include/command_reg.hpp
    ${CMAKE_SOURCE_DIR}/include/library.hpp
    ${CMAKE_SOURCE_DIR}/include/version_info.hpp
)
include_directories(${PROJECT_SOURCE_DIR}/include ${CMAKE_INSTALL_PREFIX}/include)

target_compile_options(mxcmd PRIVATE -Wall -pedantic)

if(CMD)
    find_library(Readline_LIBRARIES NAMES readline)
    find_path(Readline_INCLUDE_DIRS NAMES readline/readline.h)
    
    if(NOT Readline_LIBRARIES OR NOT Readline_INCLUDE_DIRS)
        message(WARNING "Readline not found. cmd-cli will not be built.")
        set(CMD OFF)
    else()
        add_executable(mxcmd-cli src/cmd.cpp)
        target_link_libraries(mxcmd-cli PRIVATE
            mxcmd
            ${Readline_LIBRARIES}
        )
    endif()
    target_compile_options(mxcmd-cli PRIVATE -Wall -pedantic)
endif()


if(CONSOLE)
    find_package(SDL2 REQUIRED)
    find_package(SDL2_ttf REQUIRED)
    find_package(SDL2_mixer REQUIRED)
    find_package(libmx2 REQUIRED)
    find_Package(glm REQUIRED)
    add_executable(cmd-console
        src/console_full.cpp
    )
    target_include_directories(cmd-console PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${libmx2_INCLUDE_DIRS}
        ${SDL2_INCLUDE_DIRS} 
        ${SDL2_TTF_INCLUDE_DIRS}
        ${SDL2_MIXER_INCLUDE_DIRS}
	    ${GLM_INCLUDE_DIRS}
    )
    
    target_link_libraries(cmd-console PRIVATE
        mxcmd
        libmx2::mx 
        ${SDL2_LIBRARIES} 
        ${SDL2_TTF_LIBRARIES} 
        ${SDL2_MIXER_LIBRARIES}    
    )
    target_compile_options(cmd-console PRIVATE -Wall -pedantic)
endif()

include(GNUInstallDirs)

set_target_properties(mxcmd PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    PUBLIC_HEADER "${header}"
    OUTPUT_NAME "mxcmd"  
)


install(TARGETS mxcmd
    EXPORT mxcmdTargets  
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}      
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}      
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}      
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/mxcmd  
)

install(
    FILES ${CMAKE_SOURCE_DIR}/include/3rdparty/dylib.hpp
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/mxcmd/3rdparty
)

install(EXPORT mxcmdTargets
    FILE mxcmdTargets.cmake
    NAMESPACE libmxcmd::  
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libmxcmd
)

include(CMakePackageConfigHelpers)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/libmxcmdConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/libmxcmdConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libmxcmd  
    PATH_VARS CMAKE_INSTALL_INCLUDEDIR  
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/libmxcmdConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

export(
    TARGETS mxcmd
    FILE ${CMAKE_CURRENT_BINARY_DIR}/libmxcmdTargets.cmake
    NAMESPACE libmxcmd::
)


install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/libmxcmdConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/libmxcmdConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libmxcmd
)

install(
    EXPORT mxcmdTargets
    FILE libmxcmdTargets.cmake
    NAMESPACE libmxcmd::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libmxcmd
)

if(CMD)
    install(TARGETS mxcmd-cli
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()


if(NOT TARGET uninstall)
  add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
    COMMENT "Uninstalling ${PROJECT_NAME}..."
  )
endif()


file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
"
if(NOT EXISTS \"${CMAKE_CURRENT_BINARY_DIR}/install_manifest.txt\")
  message(FATAL_ERROR \"Cannot find install manifest: ${CMAKE_CURRENT_BINARY_DIR}/install_manifest.txt\")
endif()

file(READ \"${CMAKE_CURRENT_BINARY_DIR}/install_manifest.txt\" files)
string(REGEX REPLACE \"\\n\" \";\" files \"\${files}\")

foreach(file \${files})
  message(STATUS \"Uninstalling: \${file}\")
  if(EXISTS \"\${file}\")
    execute_process(
      COMMAND \${CMAKE_COMMAND} -E remove_directory \"\${file}\"
      COMMAND \${CMAKE_COMMAND} -E remove \"\${file}\"
      OUTPUT_QUIET
      ERROR_QUIET
    )
    # Check if it was a directory and if it's now empty, try to remove it
    # This is a simplified approach; more robust directory removal might be needed
    if(IS_DIRECTORY \"\${file}\")
        # This part is tricky as 'remove_directory' might fail if not empty due to other files
        # A more robust solution would involve listing contents and removing them first
        # or using platform-specific commands.
        # For now, we rely on the fact that install_manifest.txt lists files individually.
        # If a directory is listed, it's usually because it was created by 'install(DIRECTORY...)'.
    endif()
  else()
    message(STATUS \"File not found: \${file}\")
  endif()
endforeach()
message(STATUS \"Uninstallation complete.\")
"
)
