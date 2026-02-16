# Source 폴더 및 하위 폴더의 모든 .cpp / .h 파일을 찾아 ENGINE_SOURCES에 추가
file(GLOB_RECURSE ENGINE_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.h"
)

file(GLOB_RECURSE ENGINE_PUBLIC_HEADERS CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/Include/*.h"
)

add_library(EngineCore SHARED
        ${ENGINE_SOURCES}
        ${ENGINE_PUBLIC_HEADERS}
)

# DLL export/import 매크로를 자동으로 생성
include(GenerateExportHeader)
generate_export_header(EngineCore
        EXPORT_MACRO_NAME SE_CORE_API
)


# EngineCore 라이브러리에 의존 라이브러리들을 연결
target_link_libraries(EngineCore
PUBLIC
        # tomlplusplus
        tomlplusplus::tomlplusplus

        # Tracy
        Tracy::TracyClient

        # SDL3
        SDL3::SDL3
        SDL3_image::SDL3_image
        SDL3_shadercross::SDL3_shadercross

PRIVATE
        # Project Default Options
        SE_DefaultOptions

        # ICU4C
        ICU::uc
        ICU::i18n
        ICU::data

        # cpu features
        cpu_features

        # GUID Generator
        stduuid

        # Assimp
        assimp
)

# 공개 헤더 파일의 경로를 지정 (다른 프로젝트에서 #include <SimpleEngine/Core/Engine.h> 와 같이 사용할 수 있게 하기 위함)
target_include_directories(EngineCore
PUBLIC
        # enginecore_export.h가 생성되는 빌드 디렉토리
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        # 빌드 중 사용할 공개 헤더 경로 (EngineCore/Include)
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Include>

PRIVATE
        # 내부 구현용 헤더 (EngineCore 내부 cpp 파일들이 편하게 include 하기 위함)
        ${CMAKE_CURRENT_SOURCE_DIR}/Source
)
