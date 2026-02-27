#pragma once

#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Container/Optional.h"
#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/Error/Expected.h"
#include "SimpleEngine/Core/Error/IError.h"
#include "SimpleEngine/Core/Functional/MultiDelegate.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Core/Reflection/Annotations.h"
#include "SimpleEngine/Core/Subsystem/SubsystemBase.h"

#include "SDL3/SDL.h"


namespace se
{
/** Window 생성 파라미터 */
struct WindowDesc
{
    String title = "Untitled Window";
    uint32 width = 1280;
    uint32 height = 720;
    uint32 sdl_window_flags = 0;

    // 렌더링 힌트: RenderSubsystem이 on_window_created 콜백에서 소비
    Optional<SDL_GPUSwapchainComposition> swapchain_composition = NullOpt;
    Optional<SDL_GPUPresentMode> present_mode = NullOpt;
    bool enable_hdr = false;
    bool prefer_linear_color_space = false;
};

/** Window 생성 에러 */
class SE_CORE_API WindowCreateError : public IError
{
public:
    static WindowCreateError CreationFailed(String&& sdl_error);

    [[nodiscard]] virtual const char* What() const noexcept override { return message.CStr(); }
    [[nodiscard]] virtual const IError* Source() const noexcept override { return nullptr; }

private:
    explicit WindowCreateError(String&& in_message) : message(std::move(in_message)) {}
    String message;
};

struct WindowEntry
{
    SDL_Window* native_handle = nullptr;
    WindowDesc desc;
};

/**
 * SDL 윈도우의 생명주기와 상태를 관리하는 Subsystem
 */
class SE_CORE_API SE_ANNOTATION(=meta::Internal) WindowSubsystem : public SubsystemBase
{
    SE_CLASS(WindowSubsystem, SubsystemBase)

public:
    //~ Begin SubsystemBase
    [[nodiscard]] virtual bool Initialize() override;
    virtual void Release() override;
    //~ End SubsystemBase

public:
    /**
     * 윈도우 생성 직후 Broadcast됩니다.
     * RenderSubsystem 등이 GPU Claim 및 스왑체인 설정을 수행하는 데 사용합니다.
     * @param SDL_WindowID 생성된 윈도우의 ID
     * @param SDL_Window* 생성된 네이티브 윈도우 핸들
     * @param WindowDesc& 윈도우 생성에 사용된 Description
     */
    MultiDelegate<void(SDL_WindowID, SDL_Window*, const WindowDesc&)> on_window_created;

    /**
     * 윈도우 파괴 직전 Broadcast됩니다.
     * RenderSubsystem 등이 GPU Release를 수행하는 데 사용합니다.
     * @param SDL_WindowID 파괴될 윈도우의 ID
     * @param SDL_Window* 파괴될 네이티브 윈도우 핸들
     */
    MultiDelegate<void(SDL_WindowID, SDL_Window*)> on_window_destroyed;

public:
    /** 윈도우가 키보드 포커스를 획득했을 때 Broadcast됩니다. */
    MultiDelegate<void(SDL_WindowID)> on_window_focus_gained;

    /** 윈도우가 키보드 포커스를 잃었을 때 Broadcast됩니다. */
    MultiDelegate<void(SDL_WindowID)> on_window_focus_lost;

    /** 윈도우 크기가 변경되었을 때 Broadcast됩니다. */
    MultiDelegate<void(SDL_WindowID, uint32 /*width*/, uint32 /*height*/)> on_window_resized;

    /**
     * SDL_EVENT_WINDOW_CLOSE_REQUESTED 이벤트 발생 시 Broadcast됩니다.
     * @param SDL_WindowID 닫기가 요청된 윈도우의 ID
     */
    MultiDelegate<void(SDL_WindowID)> on_window_close_requested;

public:
    /**
     * 메인 윈도우의 생성 파라미터를 설정합니다.
     * Initialize() 전에 호출해야 합니다.
     */
    void PrepareWindow(WindowDesc&& window_desc);

    /**
     * 새 윈도우를 생성합니다.
     * 성공 시 on_window_created를 Broadcast합니다.
     * @return 생성된 윈도우의 ID, 또는 실패 시 WindowCreateError
     */
    [[nodiscard]] Expected<SDL_WindowID, WindowCreateError> CreateWindow(const WindowDesc& window_desc);

    /**
     * 윈도우를 파괴합니다.
     * 파괴 직전 on_window_destroyed를 Broadcast합니다.
     * 메인 윈도우는 이 메서드로 파괴할 수 없습니다.
     * @return 파괴 성공 여부
     */
    bool DestroyWindow(SDL_WindowID window_id);

public:
    /** WindowID로 네이티브 핸들을 가져옵니다. 없으면 nullptr. */
    [[nodiscard]] SDL_Window* GetWindow(SDL_WindowID window_id) const;

    /** WindowID로 WindowDesc를 가져옵니다. 없으면 nullptr. */
    [[nodiscard]] Optional<const WindowDesc&> GetWindowDesc(SDL_WindowID window_id) const;

    /** WindowID에 해당하는 WindowEntry를 가져옵니다. 없으면 nullptr. */
    [[nodiscard]] Optional<const WindowEntry&> GetWindowEntry(SDL_WindowID window_id) const;

    /** 메인 윈도우의 ID. 메인 윈도우가 없으면 0. */
    [[nodiscard]] SDL_WindowID GetMainWindowID() const { return main_window_id; }

    /** 메인 윈도우의 네이티브 핸들. 없으면 nullptr. */
    [[nodiscard]] SDL_Window* GetMainWindow() const { return GetWindow(main_window_id); }

    /** 현재 키보드 포커스를 가진 윈도우의 ID. 없으면 0. */
    [[nodiscard]] SDL_WindowID GetFocusedWindowID() const { return focused_window_id; }

    /** 해당 윈도우가 존재하는지 확인합니다. */
    [[nodiscard]] bool HasWindow(SDL_WindowID window_id) const;

    /** 현재 관리 중인 윈도우 수를 반환합니다. */
    [[nodiscard]] uint32 GetWindowCount() const;

public:
    /**
     * 등록된 모든 윈도우에 대해 콜백을 순회 실행합니다.
     * 순회 중 CreateWindow/DestroyWindow를 호출하면 안 됩니다.
     */
    template <typename Fn>
        requires std::invocable<Fn, SDL_WindowID, SDL_Window*, const WindowDesc&>
    void ForEachWindow(Fn&& func) const;

    /** 내부 윈도우 맵에 대한 읽기 전용 접근 (range-for 지원) */
    [[nodiscard]] const auto& GetWindows() const { return windows; }

public:
    /** 특정 윈도우가 풀스크린 상태인지 확인합니다. */
    [[nodiscard]] bool IsFullscreen(SDL_WindowID window_id) const;

    /** 특정 윈도우의 풀스크린 상태를 설정합니다. */
    void SetFullscreen(SDL_WindowID window_id, bool fullscreen);

    /** 메인 윈도우가 풀스크린 상태인지 확인합니다. */
    [[nodiscard]] bool IsFullscreen() const { return IsFullscreen(main_window_id); }

    /** 메인 윈도우의 풀스크린 상태를 설정합니다. */
    void SetFullscreen(bool fullscreen) { SetFullscreen(main_window_id, fullscreen); }

private:
    void RegisterWindow(SDL_WindowID window_id, SDL_Window* window, const WindowDesc& desc);
    void UnregisterWindow(SDL_WindowID window_id);
    void OnSDLEvent(SDL_Event& event);

private:
    Optional<WindowDesc> prepared_main_window_desc = NullOpt;
    SDL_WindowID main_window_id = 0;
    SDL_WindowID focused_window_id = 0;

    HashMap<SDL_WindowID, WindowEntry> windows;

    DelegateHandle sdl_event_handle;
};

template <typename Fn>
    requires std::invocable<Fn, SDL_WindowID, SDL_Window*, const WindowDesc&>
void WindowSubsystem::ForEachWindow(Fn&& func) const
{
    for (const auto& [id, entry] : windows)
    {
        std::invoke(std::forward<Fn>(func), id, entry.native_handle, entry.desc);
    }
}
} // namespace se
