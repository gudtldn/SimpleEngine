export module SimpleEngine.Subsystems.PlatformSubsystem;

import SimpleEngine.Platform.Types;
import SimpleEngine.Core.ISubsystem;
import std;
import <SDL3/SDL.h>;


export class PlatformSubsystem : public ISubsystem
{
    struct WindowInfo
    {
        std::u8string title;
        uint32 width;
        uint32 height;
        uint32 sdl_window_flags;
    };

public:
    /**
     * PlatformSubsystem을 새로 생성합니다.
     * @param in_sdl_init_flags SDL_Init에 들어갈 Flag목록
     */
    explicit PlatformSubsystem(
        uint32 in_sdl_init_flags =
            SDL_INIT_VIDEO
            | SDL_INIT_AUDIO
            | SDL_INIT_GAMEPAD
            | SDL_INIT_EVENTS
    );

    /**
     * SDL 윈도우 초기화 및 생성 준비를 수행합니다.
     * @param window_title 창 이름
     * @param window_width 창 너비
     * @param window_height 창 높이
     * @param sdl_window_flags SDL_WindowFlags 조합 비트마스크
     */
    void PrepareWindow(
        const std::u8string& window_title,
        uint32 window_width,
        uint32 window_height,
        uint32 sdl_window_flags
    );

    [[nodiscard]] virtual bool Initialize() override;
    virtual void Release() override;

    void PollEvents(std::vector<SDL_Event>& out_events);

public:
    SDL_Window* GetWindow() const { return window; }

private:
    const uint32 sdl_init_flags;

    WindowInfo window_info;
    SDL_Window* window = nullptr;
};
