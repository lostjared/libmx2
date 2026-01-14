cmake_minimum_required(VERSION 3.10)
project(mxcmd_cxx VERSION 1.11 LANGUAGES CXX C)


include(CheckIncludeFile)
include(CheckIncludeFileCXX)
include(CheckTypeSize)
include(CheckCXXSourceCompiles)
include(CheckLibraryExists)
include(CheckSymbolExists)
include(CheckFunctionExists)

option(CMD "Build command line interface" OFF)
option(CONSOLE "Build graphical console" OFF)
option(SHARED "Build shared version of library" OFF)
option(RELEASE "Build with Optimization" OFF)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_PREFIX}/lib)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

message(STATUS "Checking C++20 compiler support...")

check_cxx_source_compiles("
#include <version>
#if !defined(__cpp_concepts) || __cpp_concepts < 201907L
#error \"Concepts not supported\"
#endif
int main() { return 0; }
" HAS_CPP20_CONCEPTS)

check_cxx_source_compiles("
#include <optional>
#include <string>
int main() {
    std::optional<std::string> opt;
    return 0;
}
" HAS_STD_OPTIONAL)

check_cxx_source_compiles("
#include <filesystem>
int main() {
    std::filesystem::path p;
    return 0;
}
" HAS_STD_FILESYSTEM)

check_cxx_source_compiles("
#include <variant>
int main() {
    std::variant<int, double> v;
    return 0;
}
" HAS_STD_VARIANT)

if(NOT HAS_STD_OPTIONAL)
    message(FATAL_ERROR "C++17 std::optional is required but not available")
endif()

if(NOT HAS_STD_FILESYSTEM)
    message(FATAL_ERROR "C++17 std::filesystem is required but not available")
endif()

message(STATUS "Checking type sizes...")

check_type_size("size_t" SIZE_T_SIZE)
check_type_size("uint64_t" UINT64_T_SIZE LANGUAGE CXX)
check_type_size("int64_t" INT64_T_SIZE LANGUAGE CXX)
check_type_size("uint32_t" UINT32_T_SIZE LANGUAGE CXX)
check_type_size("void*" POINTER_SIZE)

if(NOT HAVE_SIZE_T_SIZE)
    message(FATAL_ERROR "size_t type is required but not found")
endif()

if(NOT HAVE_UINT64_T_SIZE)
    message(FATAL_ERROR "uint64_t type is required but not found")
endif()

message(STATUS "  size_t size: ${SIZE_T_SIZE} bytes")
message(STATUS "  uint64_t size: ${UINT64_T_SIZE} bytes")
message(STATUS "  pointer size: ${POINTER_SIZE} bytes")

message(STATUS "Checking required C++ headers...")

set(REQUIRED_CXX_HEADERS
    iostream
    fstream
    sstream
    string
    vector
    optional
    memory
    functional
    unordered_map
    regex
    iomanip
    cstdlib
    random
    thread
    mutex
    atomic
    filesystem
    set
)

foreach(header ${REQUIRED_CXX_HEADERS})
    string(TOUPPER "${header}" header_upper)
    string(REPLACE "/" "_" header_upper "${header_upper}")
    check_include_file_cxx("${header}" HAS_HEADER_${header_upper})
    if(NOT HAS_HEADER_${header_upper})
        message(FATAL_ERROR "Required C++ header <${header}> not found")
    endif()
endforeach()
message(STATUS "  All required C++ headers found")

message(STATUS "Checking platform-specific headers...")

if(NOT WIN32 AND NOT EMSCRIPTEN)
    check_include_file("unistd.h" HAVE_UNISTD_H)
    check_include_file("sys/wait.h" HAVE_SYS_WAIT_H)
    check_include_file("signal.h" HAVE_SIGNAL_H)
    check_include_file("fcntl.h" HAVE_FCNTL_H)
    check_include_file("sys/select.h" HAVE_SYS_SELECT_H)
    check_include_file("pty.h" HAVE_PTY_H)
    check_include_file("sys/stat.h" HAVE_SYS_STAT_H)
    check_include_file("util.h" HAVE_UTIL_H)
    
    if(NOT HAVE_UNISTD_H)
        message(WARNING "unistd.h not found - some POSIX features may be unavailable")
    endif()
    
    if(NOT HAVE_PTY_H AND NOT HAVE_UTIL_H)
        message(WARNING "Neither pty.h nor util.h found - PTY support may be limited")
    endif()
    
    check_function_exists(fork HAVE_FORK)
    check_function_exists(pipe HAVE_PIPE)
    check_function_exists(waitpid HAVE_WAITPID)
    
    if(NOT HAVE_FORK OR NOT HAVE_PIPE)
        message(WARNING "Some POSIX process functions not available")
    endif()
endif()

if(WIN32)
    check_include_file("windows.h" HAVE_WINDOWS_H)
    if(NOT HAVE_WINDOWS_H)
        message(FATAL_ERROR "windows.h is required on Windows but not found")
    endif()
endif()

message(STATUS "Checking threading support...")

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

if(CMAKE_USE_PTHREADS_INIT)
    message(STATUS "  Using pthreads")
elseif(CMAKE_USE_WIN32_THREADS_INIT)
    message(STATUS "  Using Win32 threads")
else()
    message(STATUS "  Using native threading")
endif()

check_cxx_source_compiles("
#include <filesystem>
int main() {
    std::filesystem::path p = std::filesystem::current_path();
    return p.empty() ? 1 : 0;
}
" FILESYSTEM_NO_LINK_NEEDED)

if(NOT FILESYSTEM_NO_LINK_NEEDED)
    check_library_exists(stdc++fs "" "" HAVE_STDCXXFS)
    check_library_exists(c++fs "" "" HAVE_CXXFS)
    
    if(HAVE_STDCXXFS)
        set(FILESYSTEM_LIBRARY stdc++fs)
        message(STATUS "  Linking with stdc++fs for filesystem support")
    elseif(HAVE_CXXFS)
        set(FILESYSTEM_LIBRARY c++fs)
        message(STATUS "  Linking with c++fs for filesystem support")
    endif()
endif()

if(NOT WIN32)
    check_include_file("dlfcn.h" HAVE_DLFCN_H)
    check_library_exists(dl dlopen "" HAVE_LIBDL)
    
    if(HAVE_DLFCN_H AND HAVE_LIBDL)
        set(DL_LIBRARY dl)
        message(STATUS "  Dynamic loading support: available (libdl)")
    else()
        # Some systems (like FreeBSD) have dlopen in libc
        check_function_exists(dlopen HAVE_DLOPEN_IN_LIBC)
        if(HAVE_DLOPEN_IN_LIBC)
            set(DL_LIBRARY "")
            message(STATUS "  Dynamic loading support: available (libc)")
        else()
            message(WARNING "Dynamic loading (dlopen) not available - plugin support may be limited")
        endif()
    endif()
endif()

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

target_link_libraries(mxcmd PUBLIC Threads::Threads)

if(DEFINED FILESYSTEM_LIBRARY)
    target_link_libraries(mxcmd PUBLIC ${FILESYSTEM_LIBRARY})
endif()

if(DEFINED DL_LIBRARY AND DL_LIBRARY)
    target_link_libraries(mxcmd PUBLIC ${DL_LIBRARY})
endif()

if(CMD)
    message(STATUS "Checking readline library for CLI...")
    
    find_library(Readline_LIBRARIES NAMES readline)
    find_path(Readline_INCLUDE_DIRS NAMES readline/readline.h)
    
    if(NOT Readline_LIBRARIES OR NOT Readline_INCLUDE_DIRS)
        message(WARNING "Readline not found. cmd-cli will not be built.")
        message(STATUS "  To install readline:")
        message(STATUS "    Debian/Ubuntu: sudo apt-get install libreadline-dev")
        message(STATUS "    Fedora/RHEL:   sudo dnf install readline-devel")
        message(STATUS "    Arch Linux:    sudo pacman -S readline")
        message(STATUS "    macOS:         brew install readline")
        set(CMD OFF)
    else()
        message(STATUS "  Readline found: ${Readline_LIBRARIES}")
        message(STATUS "  Readline includes: ${Readline_INCLUDE_DIRS}")
        
        set(CMAKE_REQUIRED_LIBRARIES ${Readline_LIBRARIES})
        set(CMAKE_REQUIRED_INCLUDES ${Readline_INCLUDE_DIRS})
        
        check_function_exists(readline HAVE_READLINE)
        check_function_exists(add_history HAVE_ADD_HISTORY)
        
        if(NOT HAVE_READLINE)
            message(WARNING "readline() function not found in library")
            set(CMD OFF)
        else()
            check_include_file("readline/history.h" HAVE_READLINE_HISTORY_H)
            if(NOT HAVE_READLINE_HISTORY_H)
                message(WARNING "readline/history.h not found - history support may be limited")
            endif()
            
            add_executable(mxcmd-cli src/cmd.cpp)
            target_include_directories(mxcmd-cli PRIVATE ${Readline_INCLUDE_DIRS})
            target_link_libraries(mxcmd-cli PRIVATE
                mxcmd
                ${Readline_LIBRARIES}
            )
            target_compile_options(mxcmd-cli PRIVATE -Wall -pedantic)
        endif()
        
        unset(CMAKE_REQUIRED_LIBRARIES)
        unset(CMAKE_REQUIRED_INCLUDES)
    endif()

if(WIN32)
  set(RESOURCE_FILE "${CMAKE_CURRENT_SOURCE_DIR}/data/icon.rc")
  set_source_files_properties(
    ${RESOURCE_FILE}
    PROPERTIES
      LANGUAGE RC
  )
  target_sources(mxcmd-cli PRIVATE ${RESOURCE_FILE})
  target_compile_definitions(mxcmd-cli PRIVATE WINDOWS_MODE)

endif()

endif()


if(CONSOLE)
    message(STATUS "Checking dependencies for graphical console...")
    
    find_package(SDL2 QUIET)
    if(NOT SDL2_FOUND)
        message(FATAL_ERROR "SDL2 is required for CONSOLE but not found.\n"
                "  To install SDL2:\n"
                "    Debian/Ubuntu: sudo apt-get install libsdl2-dev\n"
                "    Fedora/RHEL:   sudo dnf install SDL2-devel\n"
                "    Arch Linux:    sudo pacman -S sdl2\n"
                "    macOS:         brew install sdl2")
    endif()
    message(STATUS "  SDL2 found: ${SDL2_LIBRARIES}")
    
    find_package(SDL2_ttf QUIET)
    if(NOT SDL2_ttf_FOUND)
        message(FATAL_ERROR "SDL2_ttf is required for CONSOLE but not found.\n"
                "  To install SDL2_ttf:\n"
                "    Debian/Ubuntu: sudo apt-get install libsdl2-ttf-dev\n"
                "    Fedora/RHEL:   sudo dnf install SDL2_ttf-devel\n"
                "    Arch Linux:    sudo pacman -S sdl2_ttf\n"
                "    macOS:         brew install sdl2_ttf")
    endif()
    message(STATUS "  SDL2_ttf found")
    
    find_package(SDL2_mixer QUIET)
    if(NOT SDL2_mixer_FOUND)
        message(FATAL_ERROR "SDL2_mixer is required for CONSOLE but not found.\n"
                "  To install SDL2_mixer:\n"
                "    Debian/Ubuntu: sudo apt-get install libsdl2-mixer-dev\n"
                "    Fedora/RHEL:   sudo dnf install SDL2_mixer-devel\n"
                "    Arch Linux:    sudo pacman -S sdl2_mixer\n"
                "    macOS:         brew install sdl2_mixer")
    endif()
    message(STATUS "  SDL2_mixer found")
    
    find_package(libmx2 QUIET)
    if(NOT libmx2_FOUND)
        message(FATAL_ERROR "libmx2 is required for CONSOLE but not found.\n"
                "  Please build and install libmx2 first.")
    endif()
    message(STATUS "  libmx2 found")
    
    find_package(glm QUIET)
    if(NOT glm_FOUND)
        message(FATAL_ERROR "GLM is required for CONSOLE but not found.\n"
                "  To install GLM:\n"
                "    Debian/Ubuntu: sudo apt-get install libglm-dev\n"
                "    Fedora/RHEL:   sudo dnf install glm-devel\n"
                "    Arch Linux:    sudo pacman -S glm\n"
                "    macOS:         brew install glm")
    endif()
    message(STATUS "  GLM found")
    
    find_package(OpenGL QUIET)
    if(OpenGL_FOUND)
        message(STATUS "  OpenGL found")
    else()
        message(WARNING "OpenGL not found - some features may not work")
    endif()
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
    if(WIN32)
        set_target_properties(cmd-console PROPERTIES WIN32_EXECUTABLE TRUE)
    endif()
    
    target_link_libraries(cmd-console PRIVATE
        mxcmd
        libmx2::mx 
        ${SDL2_LIBRARIES} 
        ${SDL2_TTF_LIBRARIES} 
        ${SDL2_MIXER_LIBRARIES}    
    )
    target_compile_options(cmd-console PRIVATE -Wall -pedantic)
if(WIN32)
  set(RESOURCE_FILE "${CMAKE_CURRENT_SOURCE_DIR}/data/icon.rc")
  set_source_files_properties(
    ${RESOURCE_FILE}
    PROPERTIES
      LANGUAGE RC
  )
  target_sources(cmd-console PRIVATE ${RESOURCE_FILE})
endif()

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

message(STATUS "")
message(STATUS "============================================================")
message(STATUS "mxcmd Configuration Summary")
message(STATUS "============================================================")
message(STATUS "  Version:           ${PROJECT_VERSION}")
message(STATUS "  Build type:        ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ Standard:      ${CMAKE_CXX_STANDARD}")
message(STATUS "  Install prefix:    ${CMAKE_INSTALL_PREFIX}")
message(STATUS "")
message(STATUS "  Library type:      $<IF:$<BOOL:${SHARED}>,SHARED,STATIC>")
if(SHARED)
    message(STATUS "  Library type:      SHARED")
else()
    message(STATUS "  Library type:      STATIC")
endif()
message(STATUS "")
message(STATUS "  Build CLI (CMD):   ${CMD}")
message(STATUS "  Build Console:     ${CONSOLE}")
message(STATUS "")
message(STATUS "  Platform:")
message(STATUS "    System:          ${CMAKE_SYSTEM_NAME}")
message(STATUS "    Processor:       ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "    Pointer size:    ${POINTER_SIZE} bytes")
message(STATUS "")
message(STATUS "  Detected Features:")
if(HAS_CPP20_CONCEPTS)
    message(STATUS "    C++20 Concepts:  YES")
else()
    message(STATUS "    C++20 Concepts:  NO")
endif()
message(STATUS "    std::filesystem: YES")
message(STATUS "    std::optional:   YES")
if(DEFINED FILESYSTEM_LIBRARY)
    message(STATUS "    Filesystem lib:  ${FILESYSTEM_LIBRARY}")
endif()
if(DEFINED DL_LIBRARY AND DL_LIBRARY)
    message(STATUS "    Dynamic loading: YES (${DL_LIBRARY})")
elseif(HAVE_DLOPEN_IN_LIBC)
    message(STATUS "    Dynamic loading: YES (libc)")
endif()
message(STATUS "============================================================")
message(STATUS "")
