#pragma once

#include "SimpleEngine/Core/Container/FixedArray.h"
#include "SimpleEngine/Core/Functional/MultiDelegate.h"
#include "SimpleEngine/Core/Input/KeyCode.h"
#include "SimpleEngine/Core/Input/MouseButton.h"
#include "SimpleEngine/Core/Subsystem/SubsystemBase.h"

#include "SDL3/SDL.h"


namespace se
{
/**
 * 입력 상태를 관리하는 Subsystem
 *
 * 매 프레임 BeginFrame()을 호출하여 이전 프레임의 입력 상태를 갱신하고,
 * PollEvents() 과정에서 on_sdl_event를 통해 SDL 입력 이벤트를 수신하여, 현재 프레임의 입력 상태를 누적합니다.
 */
class SE_CORE_API SE_ANNOTATION(=meta::Internal) InputSubsystem : public SubsystemBase
{
    SE_CLASS(InputSubsystem, SubsystemBase)

public:
    InputSubsystem() = default;

    //~ Begin SubsystemBase
    [[nodiscard]] virtual bool Initialize() override;
    virtual void Release() override;
    //~ End SubsystemBase

public:
    /**
     * 프레임 시작 시 호출됩니다. PollEvents() 전에 호출되어야 합니다.
     * 이전 프레임의 현재 상태를 '이전 상태'로 복사하고, 마우스 델타를 초기화합니다.
     */
    void BeginFrame();

public:
    // Keyboard Event

    /** 키가 현재 프레임에서 눌려있는지 확인합니다. */
    [[nodiscard]] bool IsKeyDown(EKeyCode key) const;

    /** 키가 이번 프레임에 새로 눌렸는지 확인합니다. (이전 프레임에 안 눌림 -> 현재 눌림) */
    [[nodiscard]] bool IsKeyPressed(EKeyCode key) const;

    /** 키가 이번 프레임에 떼어졌는지 확인합니다. (이전 프레임에 눌림 -> 현재 안 눌림) */
    [[nodiscard]] bool IsKeyReleased(EKeyCode key) const;

    /** 지정된 modifier가 모두 눌려있는지 확인합니다. (AND 시맨틱) */
    [[nodiscard]] bool HasModifier(BitFlags<EModifier> modifiers) const;

    /** 지정된 modifier 중 하나라도 눌려있는지 확인합니다. (OR 시맨틱) */
    [[nodiscard]] bool HasAnyModifier(BitFlags<EModifier> modifiers) const;

public:
    // Mouse Event

    /** 마우스 버튼이 현재 프레임에서 눌려있는지 확인합니다. */
    [[nodiscard]] bool IsMouseButtonDown(EMouseButton button) const;

    /** 마우스 버튼이 이번 프레임에 새로 눌렸는지 확인합니다. */
    [[nodiscard]] bool IsMouseButtonPressed(EMouseButton button) const;

    /** 마우스 버튼이 이번 프레임에 떼어졌는지 확인합니다. */
    [[nodiscard]] bool IsMouseButtonReleased(EMouseButton button) const;

    /** 모니터 전체 기준 마우스의 절대 좌표를 가져옵니다. */
    [[nodiscard]] Vector2f GetGlobalMousePosition() const;

    /** 모니터 전체 기준 특정 좌표로 마우스 커서를 이동시킵니다. */
    void SetGlobalMousePosition(const Vector2f& pos);

    /** 포커싱된 현재 윈도우 기준 마우스의 현재 좌표를 가져옵니다. */
    [[nodiscard]] Vector2f GetLocalMousePosition() const { return mouse_position; }

    /** 윈도우 내 특정 좌표로 마우스 커서를 이동시킵니다. */
    void SetLocalMousePosition(const Vector2f& pos);

    /** 이전 프레임 대비 마우스 이동량을 가져옵니다. */
    [[nodiscard]] Vector2f GetMouseDelta() const { return mouse_delta; }

    /** 이번 프레임의 마우스 휠 스크롤량을 가져옵니다. */
    [[nodiscard]] Vector2f GetMouseWheel() const { return mouse_wheel; }

public:
    // Cursor Management

    /** 커서를 숨기거나 보여줍니다. */
    void SetCursorVisible(bool visible);

    /** 커서가 현재 보이는 상태인지 확인합니다. */
    [[nodiscard]] bool IsCursorVisible() const;

    /** 포커스된 윈도우에 상대 마우스 모드를 설정합니다. (커서 잠금 + 무한 이동) */
    void SetRelativeMouseMode(bool enabled);

    /** 특정 윈도우에 상대 마우스 모드를 설정합니다. */
    void SetRelativeMouseMode(SDL_WindowID window_id, bool enabled);

    /** 포커스된 윈도우의 상대 마우스 모드가 활성화되어 있는지 확인합니다. */
    [[nodiscard]] bool IsRelativeMouseMode() const;

    /** 특정 윈도우의 상대 마우스 모드가 활성화되어 있는지 확인합니다. */
    [[nodiscard]] bool IsRelativeMouseMode(SDL_WindowID window_id) const;

private:
    /** EventSubsystem의 on_sdl_event에 등록되는 콜백입니다. */
    void OnSDLEvent(const SDL_Event& event);

private:
    static constexpr uint16 KEY_COUNT = static_cast<uint16>(EKeyCode::Max);
    static constexpr uint8 MOUSE_BUTTON_COUNT = static_cast<uint8>(EMouseButton::Max);

    // Keyboard State
    FixedArray<bool, KEY_COUNT> current_keys = {};
    FixedArray<bool, KEY_COUNT> previous_keys = {};

    // Mouse Button State
    FixedArray<bool, MOUSE_BUTTON_COUNT> current_mouse_buttons = {};
    FixedArray<bool, MOUSE_BUTTON_COUNT> previous_mouse_buttons = {};

    // Mouse Position / Delta
    Vector2f mouse_position = Vector2f::Zero();
    Vector2f mouse_delta = Vector2f::Zero();
    Vector2f mouse_wheel = Vector2f::Zero();

    // Delegate Handle
    DelegateHandle sdl_event_handle;
};
} // namespace se
