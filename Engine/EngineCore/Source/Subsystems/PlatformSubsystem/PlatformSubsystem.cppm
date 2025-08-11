export module SimpleEngine.Subsystems.PlatformSubsystem;
export import :PlatformEventDispatcher;

import SimpleEngine.Types;
import SimpleEngine.Interface.ISubsystem;
import std;
import <SDL3/SDL.h>;
import <SDL3/SDL_init.h>;


export struct WindowDesc
{
    std::u8string title = u8"Untitled Window";
    uint32 width = 1280;
    uint32 height = 720;
    uint32 sdl_window_flags = 0;

    Optional<SDL_GPUSwapchainComposition> swapchain_composition = std::nullopt;
    Optional<SDL_GPUPresentMode> present_mode = std::nullopt;

    // HDR 및 고급 색공간 설정
    bool enable_hdr = false;
    bool prefer_linear_color_space = false;
};

export struct WindowCreateError
{
    enum class Type
    {
        WindowCreationFailed,
        GPUDeviceClaimFailed,
        SwapchainSetupFailed,
    };

    Type type;
    std::u8string message;

    static WindowCreateError WindowCreation(std::u8string&& sdl_error)
    {
        return { Type::WindowCreationFailed, std::move(sdl_error) };
    }

    static WindowCreateError GPUDeviceClaim(std::u8string&& sdl_error)
    {
        return { Type::GPUDeviceClaimFailed, std::move(sdl_error) };
    }

    static WindowCreateError SwapchainSetup(std::u8string&& sdl_error)
    {
        return { Type::SwapchainSetupFailed, std::move(sdl_error) };
    }
};

export class PlatformSubsystem : public ISubsystem<>
{
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
    std::expected<SDL_WindowID, WindowCreateError> CreateWindow(const WindowDesc& window_desc);

    /** Window를 제거합니다. (메인 윈도우는 제거할 수 없음) */
    bool DestroyWindow(SDL_WindowID window_id);

    /** WindowID로 Window 정보를 가져옵니다. */
    SDL_Window* GetWindow(SDL_WindowID window_id) const;

    /** Main Window의 ID를 가져옵니다. */
    SDL_WindowID GetMainWindowID() const { return main_window_id; }

    /** Main Window를 가져옵니다. */
    SDL_Window* GetMainWindow() const { return GetWindow(main_window_id); }

    /** 현재 관리되고 있는 모든 Window를 가져옵니다. */
    const auto& GetWindows() const { return windows; }

private:
    void RegisterWindow(SDL_WindowID window_id, SDL_Window* window);
    void UnregisterWindow(SDL_WindowID window_id);

    static SDL_GPUSwapchainComposition DetermineBestSwapchainComposition(SDL_GPUDevice* device, SDL_Window* window, const WindowDesc& desc);
    static SDL_GPUPresentMode DetermineBestPresentMode(SDL_GPUDevice* device, SDL_Window* window);

private:
    const uint32 sdl_init_flags;

    Optional<WindowDesc> main_window_info = std::nullopt;
    SDL_WindowID main_window_id = 0;

    std::unordered_map<SDL_WindowID, SDL_Window*> windows;
};
