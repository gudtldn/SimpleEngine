# Source 폴더 및 하위 폴더의 모든 .cpp 파일을 찾아 ENGINE_SOURCES에 추가
file(GLOB_RECURSE ENGINE_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp")

# EngineCore를 동적 라이브러리(.dll)로 정의
add_library(EngineCore SHARED
        ${ENGINE_SOURCES}
)

# C++17 이상에서 SHARED 라이브러리를 빌드할 때 필요한 프로퍼티 설정
set_target_properties(EngineCore PROPERTIES
        CXX_STANDARD 23
        CXX_EXTENSIONS OFF
        CXX_STANDARD_REQUIRED ON
)

# DLL export/import 매크로를 자동으로 생성
include(GenerateExportHeader)
generate_export_header(EngineCore
        EXPORT_MACRO_NAME SE_CORE_API
)


# 외부 라이브러리 관리
find_package(ICU REQUIRED COMPONENTS uc i18n data)
find_package(SDL3 REQUIRED)
find_package(SDL3_image REQUIRED)
find_package(stduuid REQUIRED)

# EngineCore 라이브러리에 의존 라이브러리들을 연결
target_link_libraries(EngineCore PUBLIC
        # tomlplusplus
        tomlplusplus::tomlplusplus

        # Tracy
        Tracy::TracyClient

        # SDL3
        SDL3::SDL3
        SDL3_image::SDL3_image
        SDL3_shadercross::SDL3_shadercross
)
target_link_libraries(EngineCore PRIVATE
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
target_include_directories(EngineCore PUBLIC
        # 설치 시 사용할 include 경로
        $<INSTALL_INTERFACE:Include>
        # enginecore_export.h가 생성되는 빌드 디렉토리
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        # 빌드 중 사용할 공개 헤더 경로 (EngineCore/Include)
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Include>
)

target_compile_definitions(EngineCore PRIVATE
        PROJECT_ROOT_DIR="${PROJECT_SOURCE_DIR}"
)

if(MSVC)
    target_compile_options(EngineCore PRIVATE
            /utf-8           # 소스/실행 파일 인코딩을 UTF-8로 사용
            /Zc:preprocessor # MSVC 전처리기 최신 표준 기능을 활성화
            /Zc:__cplusplus  # 표준 __cplusplus 버전을 사용
            /W4              # 경고 수준을 최고 수준으로 설정
            /MP              # 멀티스레드 빌드 사용

            # 예외 및 RTTI 끄기
#            /EHs-c-          # C++ 예외 처리 비활성화 (SEH 예외도 끔)
#            /GR-             # RTTI(Run-Time Type Information) 비활성화

            # 경고 무시
            /wd4251 /wd4275 /wd4324 /wd4702

            # AVX를 명시적으로 타겟팅 | TODO: 나중에 ON, OFF로 수정
            /arch:AVX2
    )

    # MSVC STL 예외 비활성화
#    target_compile_definitions(EngineCore PRIVATE _HAS_EXCEPTIONS=0)

else ()
    target_compile_options(EngineCore PRIVATE
            -finput-charset=UTF-8
            -fexec-charset=UTF-8

            -Wall -Wextra

            # 예외 및 RTTI 끄기
#            -fno-exceptions  # C++ 예외 처리 비활성화
#            -fno-rtti        # RTTI 비활성화

            # AVX + FMA를 명시적으로 타겟팅 | TODO: 나중에 ON, OFF로 수정
            -mavx -mfma
    )
endif ()
