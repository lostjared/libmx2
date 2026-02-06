#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "libmx2::mx" for configuration "Release"
set_property(TARGET libmx2::mx APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(libmx2::mx PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libmx.so"
  IMPORTED_SONAME_RELEASE "libmx.so"
  )

list(APPEND _cmake_import_check_targets libmx2::mx )
list(APPEND _cmake_import_check_files_for_libmx2::mx "${_IMPORT_PREFIX}/lib/libmx.so" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
