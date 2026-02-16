# EditorApp - Editor 실행 파일 (Launcher)
add_executable(EditorApp
        Source/Launch.cpp
)

target_link_libraries(EditorApp PRIVATE
        # Project Default Options
        SE_DefaultOptions

        # Editor DLL (EngineCore가 PUBLIC으로 전파됨)
        Editor

        # SDL3 (SDL_SetAppMetadata 사용)
        SDL3::SDL3
)
