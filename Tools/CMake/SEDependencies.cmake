# =============================================================================
# Tools/CMake/SEDependencies.cmake
# 모든 외부 패키지를 한 곳에서 찾고, 서브 프로젝트(ThirdParty) 빌드를 수행
# root CMakeLists.txt에서 include하여 사용
# =============================================================================

# vcpkg 패키지
find_package(SDL3 REQUIRED)
find_package(SDL3_image REQUIRED)
find_package(SDL3_shadercross CONFIG REQUIRED)
find_package(stduuid REQUIRED)
find_package(GTest CONFIG REQUIRED)
find_package(benchmark CONFIG REQUIRED)
find_package(efsw CONFIG REQUIRED)

# ICU4X (Rust 기반 Unicode 라이브러리)
include(FetchContent)

FetchContent_Declare(
    Corrosion
    GIT_REPOSITORY https://github.com/corrosion-rs/corrosion.git
    GIT_TAG        v0.6.1
    GIT_SHALLOW    TRUE
)

FetchContent_MakeAvailable(Corrosion)

corrosion_import_crate(
    MANIFEST_PATH "${CMAKE_SOURCE_DIR}/ThirdParty/icu4x_bridge/Cargo.toml"
)

# Cargo.lock 변경 시 자동 reconfigure
set_property(DIRECTORY "${CMAKE_SOURCE_DIR}" APPEND PROPERTY
    CMAKE_CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/ThirdParty/icu4x_bridge/Cargo.lock"
)

# block() 안의 변수는 외부로 누출되지 않음
block(PROPAGATE icu_capi_src_SOURCE_DIR)
    file(READ "${CMAKE_SOURCE_DIR}/ThirdParty/icu4x_bridge/Cargo.lock" _lock)
    string(REPLACE "\r\n" "\n" _lock "${_lock}")

    # 버전과 checksum을 한 번에 추출 (Cargo.lock이 단일 소스)
    string(REGEX MATCH
        "name = \"icu_capi\"\nversion = \"([0-9.]+)\"\nsource = [^\n]+\nchecksum = \"([a-f0-9]+)\""
        _ "${_lock}"
    )

    # TLS_VERIFY OFF: MinGW cmake에 CA 인증서 없음
    # URL_HASH: Cargo.lock의 checksum으로 무결성 보장 (TLS와 동등한 보안 수준)
    FetchContent_Declare(
        icu_capi_src
        URL           "https://static.crates.io/crates/icu_capi/icu_capi-${CMAKE_MATCH_1}.crate"
        DOWNLOAD_NAME "icu_capi-${CMAKE_MATCH_1}.tar.gz"
        URL_HASH      SHA256=${CMAKE_MATCH_2}
        TLS_VERIFY    OFF
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(icu_capi_src)
endblock()

target_include_directories(icu4x_bridge INTERFACE
    "${icu_capi_src_SOURCE_DIR}/bindings/cpp"
)

# 로컬 서드파티 빌드
add_subdirectory(ThirdParty)
