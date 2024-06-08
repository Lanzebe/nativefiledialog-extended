#----------------------------------------------------------------
# Generated CMake target import file for configuration "RelWithDebInfo".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "nfd::nfd" for configuration "RelWithDebInfo"
set_property(TARGET nfd::nfd APPEND PROPERTY IMPORTED_CONFIGURATIONS RELWITHDEBINFO)
set_target_properties(nfd::nfd PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELWITHDEBINFO "CXX"
  IMPORTED_LOCATION_RELWITHDEBINFO "${_IMPORT_PREFIX}/lib/nfd.lib"
  )

list(APPEND _cmake_import_check_targets nfd::nfd )
list(APPEND _cmake_import_check_files_for_nfd::nfd "${_IMPORT_PREFIX}/lib/nfd.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
