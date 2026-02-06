# Install script for directory: /home/jared/gpu/gpu/libmx2/libmx/libmx

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "0")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set path to fallback-tool for dependency-resolution.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/usr/bin/objdump")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libmx.so" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libmx.so")
    file(RPATH_CHECK
         FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libmx.so"
         RPATH "/usr/local/lib")
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE SHARED_LIBRARY FILES "/home/jared/gpu/gpu/libmx2/libmx/libmx/libmx/libmx.so")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libmx.so" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libmx.so")
    file(RPATH_CHANGE
         FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libmx.so"
         OLD_RPATH "::::::::::::::"
         NEW_RPATH "/usr/local/lib")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libmx.so")
    endif()
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/mx2" TYPE FILE FILES
    "/home/jared/gpu/gpu/libmx2/libmx/libmx/mx.hpp"
    "/home/jared/gpu/gpu/libmx2/libmx/libmx/gl.hpp"
    "/home/jared/gpu/gpu/libmx2/libmx/libmx/vk.hpp"
    "/home/jared/gpu/gpu/libmx2/libmx/libmx/argz.hpp"
    "/home/jared/gpu/gpu/libmx2/libmx/libmx/util.hpp"
    "/home/jared/gpu/gpu/libmx2/libmx/libmx/jpeg.hpp"
    "/home/jared/gpu/gpu/libmx2/libmx/libmx/loadpng.hpp"
    "/home/jared/gpu/gpu/libmx2/libmx/libmx/object.hpp"
    "/home/jared/gpu/gpu/libmx2/libmx/libmx/font.hpp"
    "/home/jared/gpu/gpu/libmx2/libmx/libmx/cfg.hpp"
    "/home/jared/gpu/gpu/libmx2/libmx/libmx/tee_stream.hpp"
    "/home/jared/gpu/gpu/libmx2/libmx/libmx/texture.hpp"
    "/home/jared/gpu/gpu/libmx2/libmx/libmx/exception.hpp"
    "/home/jared/gpu/gpu/libmx2/libmx/libmx/joystick.hpp"
    "/home/jared/gpu/gpu/libmx2/libmx/libmx/wrapper.hpp"
    "/home/jared/gpu/gpu/libmx2/libmx/libmx/input.hpp"
    "/home/jared/gpu/gpu/libmx2/libmx/libmx/sound.hpp"
    "/home/jared/gpu/gpu/libmx2/libmx/libmx/model.hpp"
    "/home/jared/gpu/gpu/libmx2/libmx/libmx/console.hpp"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/mx2/glad" TYPE FILE FILES "/home/jared/gpu/gpu/libmx2/libmx/libmx/include/glad/glad.h")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/mx2/KHR" TYPE FILE FILES "/home/jared/gpu/gpu/libmx2/libmx/libmx/include/KHR/khrplatform.h")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/mx2" TYPE FILE FILES "/home/jared/gpu/gpu/libmx2/libmx/libmx/config.h")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/libmx2/libmx2Targets.cmake")
    file(DIFFERENT _cmake_export_file_changed FILES
         "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/libmx2/libmx2Targets.cmake"
         "/home/jared/gpu/gpu/libmx2/libmx/libmx/libmx/CMakeFiles/Export/f9e9693a0fd11f7c1c097e2b944e7593/libmx2Targets.cmake")
    if(_cmake_export_file_changed)
      file(GLOB _cmake_old_config_files "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/libmx2/libmx2Targets-*.cmake")
      if(_cmake_old_config_files)
        string(REPLACE ";" ", " _cmake_old_config_files_text "${_cmake_old_config_files}")
        message(STATUS "Old export file \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/libmx2/libmx2Targets.cmake\" will be replaced.  Removing files [${_cmake_old_config_files_text}].")
        unset(_cmake_old_config_files_text)
        file(REMOVE ${_cmake_old_config_files})
      endif()
      unset(_cmake_old_config_files)
    endif()
    unset(_cmake_export_file_changed)
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/libmx2" TYPE FILE FILES "/home/jared/gpu/gpu/libmx2/libmx/libmx/libmx/CMakeFiles/Export/f9e9693a0fd11f7c1c097e2b944e7593/libmx2Targets.cmake")
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/libmx2" TYPE FILE FILES "/home/jared/gpu/gpu/libmx2/libmx/libmx/libmx/CMakeFiles/Export/f9e9693a0fd11f7c1c097e2b944e7593/libmx2Targets-release.cmake")
  endif()
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "/home/jared/gpu/gpu/libmx2/libmx/libmx/libmx/install_local_manifest.txt"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
