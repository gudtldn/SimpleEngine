# RuntimeApp - 게임 런타임 실행 파일 (Launcher)
add_executable(RuntimeApp
        Source/Launch.cpp
)

target_link_libraries(RuntimeApp PRIVATE
        # Project Default Options
        SE_DefaultOptions

        # EngineCore DLL
        EngineCore

        # SDL3 (SDL_SetAppMetadata 사용)
        SDL3::SDL3
)
