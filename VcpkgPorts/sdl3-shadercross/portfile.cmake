# =============================================================================
# VcpkgPorts/sdl3-shadercross/portfile.cmake
# vcpkg overlay port for SDL_shadercross (https://github.com/libsdl-org/SDL_shadercross)
#
# 핵심 전략:
#   - SDLSHADERCROSS_VENDORED=OFF  -> vcpkg의 spirv-cross, directx-dxc 를 사용
#   - GIT_SUBMODULES ""            -> 중첩 submodule (SPIRV-*, DXC) 다운로드 차단
#   - FindDirectXShaderCompiler.cmake Patch -> vcpkg 경로에서도 dxcapi.h를 찾도록 수정
# =============================================================================

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO libsdl-org/SDL_shadercross
    REF  7b7365a86611b2a7b6462e521cf1c43a037d0970 # main @ 2026-02 (v3.0.0-dev)
    SHA512 52efd2c2507d6ae874cdc177945e15494920f11148e9e9cf8da27fb5ccacb5fcbe44581005e132a84631e9d438616aa1247b7ae23f4ef1785203cdcb08af19af
    HEAD_REF main
)

# ---------------------------------------------------------------------------
# Patch: FindDirectXShaderCompiler.cmake
# vcpkg의 directx-dxc 패키지는 헤더를 include/directx-dxc/ 에 설치하므로,
# Windows PATH_SUFFIXES에 해당 경로를 추가합니다.
# ---------------------------------------------------------------------------
vcpkg_replace_string(
    "${SOURCE_PATH}/cmake/FindDirectXShaderCompiler.cmake"
    [[find_path(DirectXShaderCompiler_INCLUDE_PATH NAMES "dxcapi.h" PATH_SUFFIXES "inc" "windows/inc" HINTS ${DirectXShaderCompiler_ROOT})]]
    [[find_path(DirectXShaderCompiler_INCLUDE_PATH NAMES "dxcapi.h" PATH_SUFFIXES "inc" "windows/inc" "include/directx-dxc" HINTS ${DirectXShaderCompiler_ROOT})]]
)

# ---------------------------------------------------------------------------
# CMake 옵션 구성
# ---------------------------------------------------------------------------
string(COMPARE EQUAL "${VCPKG_LIBRARY_LINKAGE}" "dynamic" BUILD_SHARED)
string(COMPARE EQUAL "${VCPKG_LIBRARY_LINKAGE}" "static"  BUILD_STATIC)

set(DXC_ENABLED OFF)
if("dxc" IN_LIST FEATURES)
    set(DXC_ENABLED ON)
endif()

set(CLI_ENABLED OFF)
if("cli" IN_LIST FEATURES)
    set(CLI_ENABLED ON)
endif()

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DSDLSHADERCROSS_VENDORED=OFF
        -DSDLSHADERCROSS_SHARED=${BUILD_SHARED}
        -DSDLSHADERCROSS_STATIC=${BUILD_STATIC}
        -DSDLSHADERCROSS_SPIRVCROSS_SHARED=OFF
        -DSDLSHADERCROSS_DXC=${DXC_ENABLED}
        -DSDLSHADERCROSS_CLI=${CLI_ENABLED}
        -DSDLSHADERCROSS_INSTALL=ON
        -DSDLSHADERCROSS_TESTS=OFF
        -DSDLSHADERCROSS_WERROR=OFF
)

vcpkg_cmake_install()

# ---------------------------------------------------------------------------
# 설치 정리
# ---------------------------------------------------------------------------
vcpkg_cmake_config_fixup(
        PACKAGE_NAME "SDL3_shadercross"
        CONFIG_PATH  "cmake"
)

# CLI 툴이 빌드되었으면 tools/ 로 복사
if(CLI_ENABLED)
    vcpkg_copy_tools(TOOL_NAMES shadercross AUTO_CLEAN)
endif()

# 불필요 파일 삭제
file(REMOVE_RECURSE
        "${CURRENT_PACKAGES_DIR}/debug/include"
        "${CURRENT_PACKAGES_DIR}/debug/share"
)

# 라이선스
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE.txt")
