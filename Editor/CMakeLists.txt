# Source 폴더 및 하위 폴더의 모든 .cpp 파일을 찾아 EDITOR_SOURCES에 저장
file(GLOB_RECURSE EDITOR_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp")

# Editor를 실행 파일로 정의하고, 찾은 소스 파일들을 사용
add_executable(Editor
        ${EDITOR_SOURCES}
)


# Editor가 EngineCore 및 다른 라이브러리들에 의존하도록 설정
find_package(SDL3 REQUIRED)
target_link_libraries(Editor PRIVATE
        EngineCore

        # Tracy
        Tracy::TracyClient

        # SDL3
        SDL3::SDL3
        SDL3_shadercross::SDL3_shadercross

        # ImGui
        imgui
)

target_include_directories(Editor PRIVATE
        # 내부 구현에서 짧은 경로를 사용하기 위한 경로
        ${CMAKE_CURRENT_SOURCE_DIR}/Source
)

target_compile_definitions(Editor PRIVATE
        PROJECT_ROOT_DIR="${PROJECT_SOURCE_DIR}"
)

if(MSVC)
    target_compile_options(Editor PRIVATE
            /utf-8           # 인코딩을 UTF-8로 사용
            /Zc:preprocessor # MSVC 전처리기 최신 표준 기능을 활성화
            /Zc:__cplusplus  # 표준 __cplusplus 버전을 사용
            /W4              # 경고 수준을 최고 수준으로 설정
            /MP              # 멀티스레드 빌드 사용
            /wd4251 /wd4275 /wd4324 /wd4702

            # AVX를 명시적으로 타겟팅 | 나중에 ON, OFF로 수정
            /arch:AVX2
    )
else ()
    target_compile_options(Editor PRIVATE
            -finput-charset=UTF-8
            -fexec-charset=UTF-8

            -Wall -Wextra

            # AVX + FMA를 명시적으로 타겟팅 | 나중에 ON, OFF로 수정
            -mavx -mfma
    )
endif ()

