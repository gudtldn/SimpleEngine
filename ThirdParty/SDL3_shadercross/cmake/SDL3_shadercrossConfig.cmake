# sdl3_shadercross cmake project-config input for CMakeLists.txt script

include(FeatureSummary)
set_package_properties(SDL3_shadercross PROPERTIES
    URL "https://github.com/libsdl-org/SDL_shadercross/"
    DESCRIPTION "Support SPIR-V and HLSL on various backends"
)


####### Expanded from @PACKAGE_INIT@ by configure_package_config_file() #######
####### Any changes to this file will be overwritten by the next CMake run ####
####### The input file was SDL3_shadercrossConfig.cmake.in                            ########

get_filename_component(PACKAGE_PREFIX_DIR "${CMAKE_CURRENT_LIST_DIR}/../" ABSOLUTE)

macro(check_required_components _NAME)
  foreach(comp ${${_NAME}_FIND_COMPONENTS})
    if(NOT ${_NAME}_${comp}_FOUND)
      if(${_NAME}_FIND_REQUIRED_${comp})
        set(${_NAME}_FOUND FALSE)
      endif()
    endif()
  endforeach()
endmacro()

####################################################################################

set(SDLSHADERCROSS_VENDORED          false)
set(SDLSHADERCROSS_SPIRVCROSS_SHARED ON)

set(SDL3_shadercross_FOUND ON)

set(SDL3_gpu_SHADERCROSS_REQUIRED_VERSION 3.1.3)

if(SDLSHADERCROSS_VENDORED)
    include("${CMAKE_CURRENT_LIST_DIR}/SDL3_shadercross-vendored-targets.cmake")
endif()

set(SDL3_shadercross_SDL3_shadercross-shared_FOUND FALSE)
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/SDL3_shadercross-shared-targets.cmake")
    include("${CMAKE_CURRENT_LIST_DIR}/SDL3_shadercross-shared-targets.cmake")
    set(SDL3_shadercross_SDL3_shadercross-shared_FOUND TRUE)
endif()

set(SDL3_shadercross_SDL3_shadercross-static FALSE)
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/SDL3_shadercross-static-targets.cmake")
    if(NOT SDLSHADERCROSS_VENDORED)
        set(original_cmake_module_path "${CMAKE_MODULE_PATH}")
        include(CMakeFindDependencyMacro)

        if(SDLSHADERCROSS_SPIRVCROSS_SHARED)
            find_dependency(spirv_cross_c_shared REQUIRED)
        else()
            find_package(spirv_cross_core QUIET)
            find_package(spirv_cross_glsl QUIET)
            find_package(spirv_cross_hlsl QUIET)
            find_package(spirv_cross_msl QUIET)
            find_package(spirv_cross_cpp QUIET)
            find_package(spirv_cross_reflect QUIET)
            find_package(spirv_cross_c)
        endif()

        list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}")
        include("${CMAKE_CURRENT_LIST_DIR}/sdlcpu.cmake")
        SDL_DetectTargetCPUArchitectures(SDL_CPU_NAMES)
        find_dependency(DirectXShaderCompiler)

        set(CMAKE_MODULE_PATH "${original_cmake_module_path}")
    endif()
    include("${CMAKE_CURRENT_LIST_DIR}/SDL3_shadercross-static-targets.cmake")
    set(SDL3_shadercross_SDL3_shadercross-static_FOUND TRUE)
endif()

function(_sdl_create_target_alias_compat NEW_TARGET TARGET)
    if(CMAKE_VERSION VERSION_LESS "3.18")
        # Aliasing local targets is not supported on CMake < 3.18, so make it global.
        add_library(${NEW_TARGET} INTERFACE IMPORTED)
        set_target_properties(${NEW_TARGET} PROPERTIES INTERFACE_LINK_LIBRARIES "${TARGET}")
    else()
        add_library(${NEW_TARGET} ALIAS ${TARGET})
    endif()
endfunction()

# Make sure SDL3_shadercross::SDL3_shadercross always exists
if(NOT TARGET SDL3_shadercross::SDL3_shadercross)
    if(TARGET SDL3_shadercross::SDL3_shadercross-shared)
        _sdl_create_target_alias_compat(SDL3_shadercross::SDL3_shadercross SDL3_shadercross::SDL3_shadercross-shared)
    elseif(TARGET SDL3_shadercross::SDL3_shadercross-static)
        _sdl_create_target_alias_compat(SDL3_shadercross::SDL3_shadercross SDL3_shadercross::SDL3_shadercross-static)
    endif()
endif()

check_required_components(SDL3_shadercross)
