# =============================================================================
# VcpkgPorts/sdl3-shadercross/portfile.cmake
# vcpkg overlay port for SDL_shadercross (https://github.com/libsdl-org/SDL_shadercross)
#
# 핵심 전략:
#   - SDLSHADERCROSS_VENDORED=OFF  -> vcpkg의 spirv-cross, directx-dxc를 사용
#   - GIT_SUBMODULES ""            -> 중첩 submodule (SPIRV-*, DXC) 다운로드 차단
#   - FindDirectXShaderCompiler.cmake Patch -> vcpkg 경로에서도 dxcapi.h를 찾도록 수정
# =============================================================================

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO     libsdl-org/SDL_shadercross

    # 공식 릴리스가 없어 main 브랜치의 특정 커밋을 사용 (Pre-release snapshot)
    REF      1ff05bec573988a98ef9e0260b4da44f512b8367 # main @ 2026-09 (v3.0.0-dev)
    SHA512   29c28bdb467d276752e0e1ccc319801fe30fc5dfd305bf1fb5a50c2936b468cd61f575e068659bfdb2434562d809fba24f26a0d80abc453533c43a6920dc6cb0
    HEAD_REF main
    PATCHES
        fix-directx-shader-compiler-includes.patch
        fix-dxc-unconditional-dependency.patch
)

# dxc feature 활성화 여부에 따라 SDLSHADERCROSS_DXC 옵션을 결정
if("dxc" IN_LIST FEATURES)
    set(SDLSHADERCROSS_DXC ON)
else()
    set(SDLSHADERCROSS_DXC OFF)
endif()

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DSDLSHADERCROSS_INSTALL=ON
        -DSDLSHADERCROSS_INSTALL_CMAKEDIR_ROOT=share/SDL3_shadercross
        -DSDLSHADERCROSS_INSTALL_RUNTIME=OFF
        -DSDLSHADERCROSS_SPIRVCROSS_SHARED=OFF
        -DSDLSHADERCROSS_VENDORED=OFF
        -DSDLSHADERCROSS_DXC=${SDLSHADERCROSS_DXC}
)

vcpkg_cmake_install()
if(VCPKG_TARGET_IS_WINDOWS)
    vcpkg_cmake_config_fixup(PACKAGE_NAME "SDL3_shadercross")
else()
    vcpkg_cmake_config_fixup(PACKAGE_NAME "SDL3_shadercross" CONFIG_PATH "share/SDL3_shadercross/SDL3_shadercross")
endif()

# 불필요한 파일 삭제
file(REMOVE_RECURSE
        "${CURRENT_PACKAGES_DIR}/debug/include"
        "${CURRENT_PACKAGES_DIR}/debug/share"
)

vcpkg_copy_pdbs()
vcpkg_fixup_pkgconfig()

vcpkg_copy_tools(TOOL_NAMES shadercross AUTO_CLEAN)

file(INSTALL "${CMAKE_CURRENT_LIST_DIR}/usage" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE.txt")
