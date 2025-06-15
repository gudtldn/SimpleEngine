export module SimpleEngine.Subsystems.PlatformSubsystem;
export import :PlatformEventDispatcher;

import SimpleEngine.Platform.Types;
import SimpleEngine.Core.ISubsystem;
import std;
import <SDL3/SDL.h>;
import <SDL3/SDL_init.h>;


export class PlatformSubsystem : public ISubsystem
{
    struct WindowDesc
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

    EventDispatcher& GetEventDispatcher() const
    {
        static EventDispatcher platform_event_dispatcher;
        return platform_event_dispatcher;
    }

    void PollEvents();

public:
    SDL_Window* GetWindow() const { return window; }

private:
    const uint32 sdl_init_flags;

    std::optional<WindowDesc> window_info = std::nullopt;
    SDL_Window* window = nullptr;
};
