# Source 폴더 및 하위 폴더의 모든 .cpp 파일을 찾아 EDITOR_SOURCES에 저장
file(GLOB_RECURSE EDITOR_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp")

# Editor를 실행 파일로 정의하고, 찾은 소스 파일들을 사용
add_executable(Editor
        ${EDITOR_SOURCES}
)


# Editor가 EngineCore 및 다른 라이브러리들에 의존하도록 설정
find_package(SDL3 REQUIRED)
target_link_libraries(Editor PRIVATE
        # Project Default Options
        SE_DefaultOptions

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
