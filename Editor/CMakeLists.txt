include(GenerateExportHeader)

# Source 폴더 및 하위 폴더의 모든 .cpp / .h 파일을 찾아 EDITOR_SOURCES에 저장
file(GLOB_RECURSE EDITOR_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.h"
)
file(GLOB_RECURSE EDITOR_PUBLIC_HEADERS CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/Include/*.h"
)

# Editor를 SHARED 라이브러리로 정의
add_library(Editor SHARED
        ${EDITOR_SOURCES}
        ${EDITOR_PUBLIC_HEADERS}
)

# PCH 설정
target_precompile_headers(Editor PRIVATE
        "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/Include/SimpleEditor/EditorPCH.h>"
)

# DLL Export 매크로 생성 (SE_EDITOR_API)
generate_export_header(Editor
        EXPORT_MACRO_NAME SE_EDITOR_API
        EXPORT_FILE_NAME Include/SimpleEditor/EditorAPI.h
)


# Editor가 EngineCore 및 다른 라이브러리들에 의존하도록 설정
target_link_libraries(Editor
PUBLIC
        # EngineCore는 Editor를 사용하는 쪽에서도 필요
        EngineCore

PRIVATE
        # Project Default Options
        SE_DefaultOptions

        # Assimp
        assimp

        # Tracy
        Tracy::TracyClient

        # SDL3
        SDL3::SDL3
        SDL3_shadercross::SDL3_shadercross

        # ImGui
        imgui

        # File system watcher
        efsw::efsw
)

target_include_directories(Editor
PUBLIC
        # 공개 헤더 경로 (SimpleEditor/ prefix로 접근)
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Include>

        # GenerateExportHeader가 생성하는 EditorAPI.h 경로
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/Include>

PRIVATE
        # 내부 구현에서 짧은 경로를 사용하기 위한 경로
        ${CMAKE_CURRENT_SOURCE_DIR}/Source
)
