#----------------------------------------------------------------
# Generated CMake target import file for configuration "MinSizeRel".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "nfd::nfd" for configuration "MinSizeRel"
set_property(TARGET nfd::nfd APPEND PROPERTY IMPORTED_CONFIGURATIONS MINSIZEREL)
set_target_properties(nfd::nfd PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_MINSIZEREL "CXX"
  IMPORTED_LOCATION_MINSIZEREL "${_IMPORT_PREFIX}/lib/nfd.lib"
  )

list(APPEND _cmake_import_check_targets nfd::nfd )
list(APPEND _cmake_import_check_files_for_nfd::nfd "${_IMPORT_PREFIX}/lib/nfd.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
