#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "SDL3_shadercross::SDL3_shadercross-shared" for configuration "Release"
set_property(TARGET SDL3_shadercross::SDL3_shadercross-shared APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(SDL3_shadercross::SDL3_shadercross-shared PROPERTIES
  IMPORTED_IMPLIB_RELEASE "${_IMPORT_PREFIX}/lib/SDL3_shadercross.lib"
  IMPORTED_LINK_DEPENDENT_LIBRARIES_RELEASE "SDL3::SDL3-shared;spirv-cross-c-shared;DirectXShaderCompiler::dxcompiler"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/SDL3_shadercross.dll"
  )

list(APPEND _cmake_import_check_targets SDL3_shadercross::SDL3_shadercross-shared )
list(APPEND _cmake_import_check_files_for_SDL3_shadercross::SDL3_shadercross-shared "${_IMPORT_PREFIX}/lib/SDL3_shadercross.lib" "${_IMPORT_PREFIX}/bin/SDL3_shadercross.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
