cmake_minimum_required(VERSION 3.28)

# VCPKG_ROOT 환경변수가 설정된 경우 자동으로 vcpkg 툴체인 적용
# CMakeUserPresets.json이나 -DCMAKE_TOOLCHAIN_FILE= 로 직접 지정한 경우 우선됨
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "vcpkg toolchain (auto-detected from VCPKG_ROOT)")
endif()

# 커스텀 빌드 구성 정의
# Multi-config 생성기용 (Visual Studio, Ninja Multi-Config)
get_property(_is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(_is_multi_config)
    set(CMAKE_CONFIGURATION_TYPES "Debug;Development;Release" CACHE STRING "" FORCE)
endif()
unset(_is_multi_config)

# Single-config 생성기용 (Ninja, Makefile) - 유효 값 목록
set(CMAKE_BUILD_TYPE "${CMAKE_BUILD_TYPE}" CACHE STRING "Build type" FORCE)
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Development" "Release")


# 프로젝트 정의
project(SimpleEngine
        VERSION 0.1.0
        LANGUAGES C CXX
)

# Development에 Release의 컴파일/링크 플래그 복제
set(CMAKE_C_FLAGS_DEVELOPMENT             "${CMAKE_C_FLAGS_RELEASE}"             CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS_DEVELOPMENT           "${CMAKE_CXX_FLAGS_RELEASE}"           CACHE STRING "" FORCE)
set(CMAKE_EXE_LINKER_FLAGS_DEVELOPMENT    "${CMAKE_EXE_LINKER_FLAGS_RELEASE}"    CACHE STRING "" FORCE)
set(CMAKE_SHARED_LINKER_FLAGS_DEVELOPMENT "${CMAKE_SHARED_LINKER_FLAGS_RELEASE}" CACHE STRING "" FORCE)

# Development에서 PDB(디버그 정보) 생성
if (MSVC)
    # 컴파일러 옵션: /Zi (디버그 정보 생성)
    string(APPEND CMAKE_C_FLAGS_DEVELOPMENT   " /Zi")
    string(APPEND CMAKE_CXX_FLAGS_DEVELOPMENT " /Zi")

    # 링커 옵션: /DEBUG (PDB 생성 및 디버그 가능하게 함)
    string(APPEND CMAKE_EXE_LINKER_FLAGS_DEVELOPMENT " /DEBUG /INCREMENTAL")
    string(APPEND CMAKE_SHARED_LINKER_FLAGS_DEVELOPMENT " /DEBUG /INCREMENTAL")
endif ()

# 아키텍처 이름 설정 (x64 / x86)
if (CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(PROJECT_ARCHITECTURE "x64")
else ()
    set(PROJECT_ARCHITECTURE "x86")
endif ()

# 출력 디렉토리 설정
set(BINARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/Binaries/${CMAKE_SYSTEM_NAME}/${PROJECT_ARCHITECTURE})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${BINARY_OUTPUT_PATH}/${CMAKE_BUILD_TYPE}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BINARY_OUTPUT_PATH}/${CMAKE_BUILD_TYPE})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${BINARY_OUTPUT_PATH}/${CMAKE_BUILD_TYPE})
set(CMAKE_PDB_OUTPUT_DIRECTORY ${BINARY_OUTPUT_PATH}/${CMAKE_BUILD_TYPE})

# Visual Studio 다중 구성 생성기를 위한 설정
foreach (OUTPUT_CONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${OUTPUT_CONFIG} OUTPUT_CONFIG_UPPER)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUT_CONFIG_UPPER} ${BINARY_OUTPUT_PATH}/${OUTPUT_CONFIG}/lib)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUT_CONFIG_UPPER} ${BINARY_OUTPUT_PATH}/${OUTPUT_CONFIG})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUT_CONFIG_UPPER} ${BINARY_OUTPUT_PATH}/${OUTPUT_CONFIG})
    set(CMAKE_PDB_OUTPUT_DIRECTORY_${OUTPUT_CONFIG_UPPER} ${BINARY_OUTPUT_PATH}/${OUTPUT_CONFIG})
endforeach ()

# 공용 CMake 모듈 추가
include(Tools/CMake/SEDefaultOptions.cmake)
include(Tools/CMake/SEDependencies.cmake)


# 하위 프로젝트 추가 (의존성 순서)
add_subdirectory(EngineCore)
add_subdirectory(Editor)
add_subdirectory(EditorApp)
add_subdirectory(RuntimeApp)
add_subdirectory(EngineTest)
