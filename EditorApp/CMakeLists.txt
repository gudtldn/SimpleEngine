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

# D3D12 GPU 디버그 마커를 위한 WinPixEventRuntime.dll 복사 (Windows 전용)
if(WIN32)
    add_custom_command(TARGET EditorApp POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_SOURCE_DIR}/ThirdParty/WinPixEventRuntime/WinPixEventRuntime.dll
            $<TARGET_FILE_DIR:EditorApp>
            COMMENT "Copying WinPixEventRuntime.dll for GPU debug markers"
    )
endif()
