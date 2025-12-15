#pragma once
#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Container/Optional.h"
#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/Error/Expected.h"
#include "SimpleEngine/Core/Error/IError.h"
#include "SimpleEngine/Core/Event/EventDispatcher.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Core/Subsystem/ISubsystem.h"

#include "SDL3/SDL.h"


namespace se
{
struct WindowDesc
{
    se::String title = "Untitled Window";
    uint32 width = 1280;
    uint32 height = 720;
    uint32 sdl_window_flags = 0;

    Optional<SDL_GPUSwapchainComposition> swapchain_composition = std::nullopt;
    Optional<SDL_GPUPresentMode> present_mode = std::nullopt;

    // HDR 및 고급 색공간 설정
    bool enable_hdr = false;
    bool prefer_linear_color_space = false;
};

class WindowCreateError : public core::IError
{
public:
    enum class Type
    {
        WindowCreationFailed,
        GPUDeviceClaimFailed,
        SwapchainSetupFailed,
    };

    static WindowCreateError WindowCreation(String&& sdl_error) { return { Type::WindowCreationFailed, std::move(sdl_error) }; }
    static WindowCreateError GPUDeviceClaim(String&& sdl_error) { return { Type::GPUDeviceClaimFailed, std::move(sdl_error) }; }
    static WindowCreateError SwapchainSetup(String&& sdl_error) { return { Type::SwapchainSetupFailed, std::move(sdl_error) }; }

    [[nodiscard]] virtual const char* What() const noexcept override { return message.CStr(); }
    [[nodiscard]] virtual const IError* Source() const noexcept override { return nullptr; }

    [[nodiscard]] Type GetType() const noexcept { return type; }

private:
    WindowCreateError(Type in_type, String&& message)
        : type(in_type), message(std::move(message)) {}

    Type type;
    String message;
};

class SE_CORE_API PlatformSubsystem : public core::ISubsystem
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

    [[nodiscard]] core::event::EventDispatcher& GetEventDispatcher() { return platform_event_dispatcher; }
    [[nodiscard]] const core::event::EventDispatcher& GetEventDispatcher() const { return platform_event_dispatcher; }

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

    Optional<WindowDesc> main_window_info = std::nullopt;
    SDL_WindowID main_window_id = 0;

    HashMap<SDL_WindowID, SDL_Window*> windows;
    core::event::EventDispatcher platform_event_dispatcher;
};
}
