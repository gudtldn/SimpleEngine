#pragma once
#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Container/Optional.h"
#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/Error/Expected.h"
#include "SimpleEngine/Core/Error/IError.h"
#include "SimpleEngine/Core/Functional/MultiDelegate.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Core/Subsystem/SubsystemBase.h"

#include "SDL3/SDL.h"


namespace se
{
struct WindowDesc
{
    se::String title = "Untitled Window";
    uint32 width = 1280;
    uint32 height = 720;
    uint32 sdl_window_flags = 0;

    Optional<SDL_GPUSwapchainComposition> swapchain_composition = NullOpt;
    Optional<SDL_GPUPresentMode> present_mode = NullOpt;

    // HDR 및 고급 색공간 설정
    bool enable_hdr = false;
    bool prefer_linear_color_space = false;
};

class SE_CORE_API WindowCreateError : public IError
{
public:
    enum class EType
    {
        WindowCreationFailed,
        GPUDeviceClaimFailed,
        SwapchainSetupFailed,
    };

    static WindowCreateError WindowCreation(String&& sdl_error) { return { EType::WindowCreationFailed, std::move(sdl_error) }; }
    static WindowCreateError GPUDeviceClaim(String&& sdl_error) { return { EType::GPUDeviceClaimFailed, std::move(sdl_error) }; }
    static WindowCreateError SwapchainSetup(String&& sdl_error) { return { EType::SwapchainSetupFailed, std::move(sdl_error) }; }

    [[nodiscard]] virtual const char* What() const noexcept override { return message.CStr(); }
    [[nodiscard]] virtual const IError* Source() const noexcept override { return nullptr; }

    [[nodiscard]] EType GetType() const noexcept { return type; }

private:
    WindowCreateError(EType in_type, String&& message)
        : type(in_type), message(std::move(message)) {}

    EType type;
    String message;
};

/**
 * @todo docs
 */
class SE_CORE_API SE_ANNOTATION(=meta::Internal) PlatformSubsystem : public SubsystemBase
{
    SE_CLASS(PlatformSubsystem, SubsystemBase)

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

    //~ Begin SubsystemBase
    [[nodiscard]] virtual bool Initialize() override;
    virtual void Release() override;
    //~ End SubsystemBase

    void PollEvents();

public:
    /**
     * SDL 이벤트가 발생할 때마다 Broadcast됩니다.
     * ImGui 등 모든 raw SDL 이벤트를 받아야 하는 시스템용입니다.
     */
    MultiDelegate<void(SDL_Event&)> on_sdl_event;

    /**
     * SDL_EVENT_QUIT 이벤트 발생 시 Broadcast됩니다.
     */
    MultiDelegate<void()> on_quit_requested;

    /**
     * SDL_EVENT_WINDOW_CLOSE_REQUESTED 이벤트 발생 시 Broadcast됩니다.
     * @param SDL_WindowID 닫기가 요청된 윈도우의 ID
     */
    MultiDelegate<void(SDL_WindowID)> on_window_close_requested;

public:
    // TODO: 다중 윈도우에 대해 작동할 수 있도록 수?정 | Main이 아닌 Window가 Fullscreen이 필요한가?

    /** Main Window가 Fullscreen 상태인지 확인합니다. */
    [[nodiscard]] bool IsFullscreen() const;

    /** Main Window를 Fullscreen 상태로 설정합니다. */
    void SetFullscreen(bool fullscreen);

public:
    /** 메인 Window의 초기화 및 생성 준비를 수행합니다. */
    void PrepareWindow(WindowDesc&& window_desc);

    /** Window를 새로 생성합니다. */
    Expected<SDL_WindowID, WindowCreateError> CreateWindow(const WindowDesc& window_desc);

    /** Window를 제거합니다. (메인 윈도우는 제거할 수 없음) */
    bool DestroyWindow(SDL_WindowID window_id);

    /** WindowID로 Window 정보를 가져옵니다. */
    [[nodiscard]] SDL_Window* GetWindow(SDL_WindowID window_id) const;

    /** Main Window의 ID를 가져옵니다. */
    [[nodiscard]] SDL_WindowID GetMainWindowID() const { return main_window_id; }

    /** Main Window를 가져옵니다. */
    [[nodiscard]] SDL_Window* GetMainWindow() const { return GetWindow(main_window_id); }

    /** Main Window의 생성 정보를 가져옵니다. */
    [[nodiscard]] const Optional<WindowDesc>& GetMainWindowInfo() const { return main_window_info; }

    /** 현재 관리되고 있는 모든 Window를 가져옵니다. */
    [[nodiscard]] const auto& GetWindows() const { return windows; }

private:
    void RegisterWindow(SDL_WindowID window_id, SDL_Window* window);
    void UnregisterWindow(SDL_WindowID window_id);

private:
    const uint32 sdl_init_flags;

    Optional<WindowDesc> main_window_info = NullOpt;
    SDL_WindowID main_window_id = 0;

    HashMap<SDL_WindowID, SDL_Window*> windows;
};
}
