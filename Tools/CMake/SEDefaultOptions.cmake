# =============================================================================
# Tools/CMake/SEDefaultOptions.cmake
# 모든 엔진 타겟이 공유하는 컴파일 옵션을 INTERFACE 타겟으로 정의
# =============================================================================

# SE_DefaultOptions Interface Lib 추가
add_library(SE_DefaultOptions INTERFACE)
add_library(SE::DefaultOptions ALIAS SE_DefaultOptions)

# C++ 표준 설정
target_compile_features(SE_DefaultOptions INTERFACE
        cxx_std_23
)

# 컴파일러 정책 설정 (Extension 금지 및 강제성)
set_target_properties(SE_DefaultOptions PROPERTIES
        INTERFACE_CXX_EXTENSIONS OFF
        INTERFACE_CXX_STANDARD_REQUIRED ON
)

# 컴파일러 빌드 설정
target_compile_definitions(SE_DefaultOptions INTERFACE
        # 윈도우 유니코드 호환
        UNICODE _UNICODE

        # 표준 매크로 (_DEBUG / NDEBUG)
        $<$<CONFIG:Debug>:_DEBUG>
        $<$<CONFIG:Development>:NDEBUG>
        $<$<CONFIG:Release>:NDEBUG>

        # 엔진 전용 매크로
        $<$<CONFIG:Debug>:SE_CMAKE_CONFIGURATION_DEBUG>
        $<$<CONFIG:Development>:SE_CMAKE_CONFIGURATION_DEVELOPMENT>
        $<$<CONFIG:Release>:SE_CMAKE_CONFIGURATION_RELEASE>

        $<$<OR:$<CONFIG:Debug>,$<CONFIG:Development>>:SE_CMAKE_OPTION_ENABLE_ASSERTS>
)

if(MSVC)
    target_compile_options(SE_DefaultOptions INTERFACE
            /utf-8           # 소스/실행 파일 인코딩을 UTF-8로 사용
            /permissive-     # MSVC 비표준 확장 끄기
            /Zc:preprocessor # MSVC 전처리기가 표준을 준수하도록 설정
            /Zc:__cplusplus  # 표준 __cplusplus 버전을 사용
            /W4              # 경고 수준을 최고 수준으로 설정
            /MP              # 멀티스레드 빌드 사용

            # 예외 및 RTTI 끄기
            # /EHs-c-          # C++ 예외 처리 비활성화 (SEH 예외도 끔)
            # /GR-             # RTTI(Run-Time Type Information) 비활성화

            # 경고 억제
            /wd4251          # DLL 인터페이스 경고 (STL 멤버)
            /wd4275          # DLL 인터페이스 경고 (비-DLL 베이스)
            /wd4324          # 패딩 경고
            /wd4702          # unreachable code
    )

    # MSVC STL 예외 비활성화
    # target_compile_definitions(SE_DefaultOptions INTERFACE _HAS_EXCEPTIONS=0)

else()
    target_compile_options(SE_DefaultOptions INTERFACE
            -pedantic                       # 비표준 문법 경고 (MSVC /permissive- 대응)

            -finput-charset=UTF-8           # 소스 파일 인코딩을 UTF-8로 설정
            -fexec-charset=UTF-8            # 실행 파일 문자열 인코딩을 UTF-8로 설정

            -Wall -Wextra                   # 일반적인 모든 경고와 추가적인 경고까지 활성화
            -Wno-missing-field-initializers # Designated Initializer 일부 생략 허용
            -Wno-interference-size          # 캐시 라인 크기 경고 무시
            -Wno-unknown-pragmas            # 다른 컴파일러용 pragma 무시
            -Wno-init-list-lifetime         # init-list 수명 경고 무시 (ArrayView의 inline으로 사용하는 경우)

            # 예외 및 RTTI 끄기
            # -fno-exceptions               # C++ 예외 처리 비활성화
            # -fno-rtti                     # RTTI 비활성화
    )

    if(MINGW)
        target_link_libraries(SE_DefaultOptions INTERFACE
                stdc++exp                   # <print>, <format> 사용을 위한 라이브러리
        )
    endif()
endif()

# SIMD 관련 설정
include(${CMAKE_CURRENT_LIST_DIR}/SESimdOptions.cmake)
