# Source 폴더 및 하위 폴더의 모든 .cpp 파일을 찾습니다.
file(GLOB_RECURSE ALL_SOURCES "Source/*.cpp")

## 이 중에서 HAL 폴더에 있는 파일들은 제외
#list(FILTER ALL_SOURCES EXCLUDE REGEX ".*/HAL/.*\\.cpp$")
#
## 플랫폼에 따라 HAL 구현 파일을 선택적으로 추가
#if (WIN32)
#    set(PLATFORM_SOURCE_DIR "Source/Core/HAL/Windows")
#elseif (APPLE)
#    set(PLATFORM_SOURCE_DIR "Source/Core/HAL/Apple")
#elseif (UNIX)
#    set(PLATFORM_SOURCE_DIR "Source/Core/HAL/Linux")
#endif ()
#
#file(GLOB_RECURSE PLATFORM_SOURCES "${PLATFORM_SOURCE_DIR}/*.cpp")

# EngineCore를 동적 라이브러리(.dll)로 정의
add_library(EngineCore SHARED
        ${ALL_SOURCES}
        #        ${PLATFORM_SOURCES}
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

# EngineCore 라이브러리에 의존 라이브러리들을 연결
target_link_libraries(EngineCore PUBLIC
        # tomlplusplus
        tomlplusplus::tomlplusplus

        # Tracy
        Tracy::TracyClient

        # SDL3
        SDL3::SDL3
        SDL3_shadercross::SDL3_shadercross
)
target_link_libraries(EngineCore PRIVATE
        # ICU4C
        ICU::uc
        ICU::i18n
        ICU::data
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
target_include_directories(EngineCore PRIVATE
        # 내부 구현에서 짧은 경로를 사용하기 위한 경로
        ${CMAKE_CURRENT_SOURCE_DIR}/Include/SimpleEngine
)
