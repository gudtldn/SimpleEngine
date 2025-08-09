export module SimpleEngine.Subsystems.PlatformSubsystem;
export import :PlatformEventDispatcher;

import SimpleEngine.Types;
import SimpleEngine.Interface.ISubsystem;
import std;
import <SDL3/SDL.h>;
import <SDL3/SDL_init.h>;


export class PlatformSubsystem : public ISubsystem<>
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

    //~ Begin ISubsystem
    [[nodiscard]] virtual bool Initialize() override;
    virtual void Release() override;
    //~ End ISubsystem

    void PollEvents();

    PlatformEventDispatcher& GetEventDispatcher() const
    {
        static PlatformEventDispatcher platform_event_dispatcher;
        return platform_event_dispatcher;
    }

public:
    /** 메인 Window의 초기화 및 생성 준비를 수행합니다. */
    void PrepareWindow(WindowDesc&& window_desc);

    /** Window를 새로 생성합니다. */
    SDL_WindowID CreateWindow(const WindowDesc& window_desc);

    /** Window를 제거합니다. (메인 윈도우는 제거할 수 없음) */
    bool DestroyWindow(SDL_WindowID window_id);

    /** WindowID로 Window 정보를 가져옵니다. */
    SDL_Window* GetWindow(SDL_WindowID window_id) const;

    /** Main Window의 ID를 가져옵니다. */
    SDL_WindowID GetMainWindowID() const { return main_window_id; }

    /** Main Window를 가져옵니다. */
    SDL_Window* GetMainWindow() const { return GetWindow(main_window_id); }

private:
    void RegisterWindow(SDL_WindowID window_id, SDL_Window* window);
    void UnregisterWindow(SDL_WindowID window_id);

private:
    const uint32 sdl_init_flags;

    Optional<WindowDesc> main_window_info = std::nullopt;
    SDL_WindowID main_window_id = 0;

    std::unordered_map<SDL_WindowID, SDL_Window*> windows;
};
