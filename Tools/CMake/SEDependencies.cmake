# =============================================================================
# Tools/CMake/SEDependencies.cmake
# 모든 외부 패키지를 한 곳에서 찾고, 서브 프로젝트(ThirdParty) 빌드를 수행
# root CMakeLists.txt에서 include하여 사용
# =============================================================================

# vcpkg 패키지
find_package(SDL3 REQUIRED)
find_package(SDL3_image REQUIRED)
find_package(sdl3_shadercross REQUIRED)
find_package(stduuid REQUIRED)
find_package(GTest CONFIG REQUIRED)
find_package(benchmark CONFIG REQUIRED)

# HLSL 컴파일 가능 여부 판단 (DXC feature가 활성화된 shadercross 빌드인지 확인)
include(CheckSymbolExists)
set(CMAKE_REQUIRED_LIBRARIES SDL3_shadercross::SDL3_shadercross)
set(CMAKE_REQUIRED_INCLUDES "${SDL3_shadercross_INCLUDE_DIRS}")
check_symbol_exists(SDL_ShaderCross_CompileDXILFromHLSL "SDL3_shadercross/SDL_shadercross.h" SE_HAS_SHADER_COMPILER)
unset(CMAKE_REQUIRED_LIBRARIES)
unset(CMAKE_REQUIRED_INCLUDES)

message(STATUS "SimpleEngine: Shader compiler (HLSL via DXC): ${SE_HAS_SHADER_COMPILER}")

# ICU4X (Rust 기반 Unicode 라이브러리)
include(FetchContent)

FetchContent_Declare(
    Corrosion
    GIT_REPOSITORY https://github.com/corrosion-rs/corrosion.git
    GIT_TAG        v0.6.1
    GIT_SHALLOW    TRUE
)

FetchContent_Declare(
    icu4x_src
    GIT_REPOSITORY https://github.com/unicode-org/icu4x.git
    GIT_TAG        icu@2.1.0
    GIT_SHALLOW    TRUE
)

FetchContent_MakeAvailable(Corrosion icu4x_src)

corrosion_import_crate(
    MANIFEST_PATH "${icu4x_src_SOURCE_DIR}/ffi/capi/Cargo.toml"
    OVERRIDE_CRATE_TYPE icu_capi=staticlib
)

# 로컬 서드파티 빌드
add_subdirectory(ThirdParty)
