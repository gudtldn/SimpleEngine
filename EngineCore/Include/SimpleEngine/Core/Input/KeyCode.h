#pragma once

#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Core/Types/BitFlags.h"

#include "SDL3/SDL_scancode.h"


namespace se
{
/**
 * 물리적 키보드 키를 나타내는 열거형
 */
enum class EKeyCode : u16
{
    Unknown = SDL_SCANCODE_UNKNOWN,

    // ── 알파벳 ──────────────────────────────────────
    A = SDL_SCANCODE_A,
    B = SDL_SCANCODE_B,
    C = SDL_SCANCODE_C,
    D = SDL_SCANCODE_D,
    E = SDL_SCANCODE_E,
    F = SDL_SCANCODE_F,
    G = SDL_SCANCODE_G,
    H = SDL_SCANCODE_H,
    I = SDL_SCANCODE_I,
    J = SDL_SCANCODE_J,
    K = SDL_SCANCODE_K,
    L = SDL_SCANCODE_L,
    M = SDL_SCANCODE_M,
    N = SDL_SCANCODE_N,
    O = SDL_SCANCODE_O,
    P = SDL_SCANCODE_P,
    Q = SDL_SCANCODE_Q,
    R = SDL_SCANCODE_R,
    S = SDL_SCANCODE_S,
    T = SDL_SCANCODE_T,
    U = SDL_SCANCODE_U,
    V = SDL_SCANCODE_V,
    W = SDL_SCANCODE_W,
    X = SDL_SCANCODE_X,
    Y = SDL_SCANCODE_Y,
    Z = SDL_SCANCODE_Z,

    // ── 숫자 ──────────────────────────────────────
    Num0 = SDL_SCANCODE_0,
    Num1 = SDL_SCANCODE_1,
    Num2 = SDL_SCANCODE_2,
    Num3 = SDL_SCANCODE_3,
    Num4 = SDL_SCANCODE_4,
    Num5 = SDL_SCANCODE_5,
    Num6 = SDL_SCANCODE_6,
    Num7 = SDL_SCANCODE_7,
    Num8 = SDL_SCANCODE_8,
    Num9 = SDL_SCANCODE_9,

    // ── 기능 키 ──────────────────────────────────────
    F1  = SDL_SCANCODE_F1,
    F2  = SDL_SCANCODE_F2,
    F3  = SDL_SCANCODE_F3,
    F4  = SDL_SCANCODE_F4,
    F5  = SDL_SCANCODE_F5,
    F6  = SDL_SCANCODE_F6,
    F7  = SDL_SCANCODE_F7,
    F8  = SDL_SCANCODE_F8,
    F9  = SDL_SCANCODE_F9,
    F10 = SDL_SCANCODE_F10,
    F11 = SDL_SCANCODE_F11,
    F12 = SDL_SCANCODE_F12,

    // ── 수정자 키 ──────────────────────────────────────
    LeftShift  = SDL_SCANCODE_LSHIFT,
    RightShift = SDL_SCANCODE_RSHIFT,
    LeftCtrl   = SDL_SCANCODE_LCTRL,
    RightCtrl  = SDL_SCANCODE_RCTRL,
    LeftAlt    = SDL_SCANCODE_LALT,
    RightAlt   = SDL_SCANCODE_RALT,
    LeftGui    = SDL_SCANCODE_LGUI, // 운영체제 키
    RightGui   = SDL_SCANCODE_RGUI, // 운영체제 키

    // ── 특수 키 ──────────────────────────────────────
    Enter     = SDL_SCANCODE_RETURN,
    Escape    = SDL_SCANCODE_ESCAPE,
    Backspace = SDL_SCANCODE_BACKSPACE,
    Tab       = SDL_SCANCODE_TAB,
    Space     = SDL_SCANCODE_SPACE,
    CapsLock  = SDL_SCANCODE_CAPSLOCK,

    // ── 기호 / 구두점 ──────────────────────────────────────
    Minus        = SDL_SCANCODE_MINUS,        // -
    Equals       = SDL_SCANCODE_EQUALS,       // =
    LeftBracket  = SDL_SCANCODE_LEFTBRACKET,  // [
    RightBracket = SDL_SCANCODE_RIGHTBRACKET, // ]
    Backslash    = SDL_SCANCODE_BACKSLASH,    /* \ */
    Semicolon    = SDL_SCANCODE_SEMICOLON,    // ;
    Apostrophe   = SDL_SCANCODE_APOSTROPHE,   // '
    Grave        = SDL_SCANCODE_GRAVE,        // `
    Comma        = SDL_SCANCODE_COMMA,        // ,
    Period       = SDL_SCANCODE_PERIOD,       // .
    Slash        = SDL_SCANCODE_SLASH,        // /

    // ── 네비게이션 ──────────────────────────────────────
    PrintScreen = SDL_SCANCODE_PRINTSCREEN,
    ScrollLock  = SDL_SCANCODE_SCROLLLOCK,
    Pause       = SDL_SCANCODE_PAUSE,
    Insert      = SDL_SCANCODE_INSERT,
    Home        = SDL_SCANCODE_HOME,
    PageUp      = SDL_SCANCODE_PAGEUP,
    Delete      = SDL_SCANCODE_DELETE,
    End         = SDL_SCANCODE_END,
    PageDown    = SDL_SCANCODE_PAGEDOWN,

    // ── 방향키 ──────────────────────────────────────
    Right = SDL_SCANCODE_RIGHT,
    Left  = SDL_SCANCODE_LEFT,
    Down  = SDL_SCANCODE_DOWN,
    Up    = SDL_SCANCODE_UP,

    // ── 넘패드 ──────────────────────────────────────
    NumLock    = SDL_SCANCODE_NUMLOCKCLEAR,
    KpDivide   = SDL_SCANCODE_KP_DIVIDE,
    KpMultiply = SDL_SCANCODE_KP_MULTIPLY,
    KpMinus    = SDL_SCANCODE_KP_MINUS,
    KpPlus     = SDL_SCANCODE_KP_PLUS,
    KpEnter    = SDL_SCANCODE_KP_ENTER,
    Kp0        = SDL_SCANCODE_KP_0,
    Kp1        = SDL_SCANCODE_KP_1,
    Kp2        = SDL_SCANCODE_KP_2,
    Kp3        = SDL_SCANCODE_KP_3,
    Kp4        = SDL_SCANCODE_KP_4,
    Kp5        = SDL_SCANCODE_KP_5,
    Kp6        = SDL_SCANCODE_KP_6,
    Kp7        = SDL_SCANCODE_KP_7,
    Kp8        = SDL_SCANCODE_KP_8,
    Kp9        = SDL_SCANCODE_KP_9,
    KpPeriod   = SDL_SCANCODE_KP_PERIOD,

    // ── 배열 상한 (키가 아님) ──────────────────────────────
    Max = SDL_SCANCODE_COUNT,
};

/**
 * Keyboard의 Modifier를 나타내는 비트플래그
 */
enum class EModifier : u8
{
    None  = 0,
    Shift = 1 << 0,
    Ctrl  = 1 << 1,
    Alt   = 1 << 2,
    Gui   = 1 << 3,
};
SE_ENABLE_BITMASK_OPERATORS(EModifier)

/** SDL_Scancode를 KeyCode로 변환합니다. */
[[nodiscard]] constexpr EKeyCode ToKeyCode(SDL_Scancode scancode) noexcept
{
    return static_cast<EKeyCode>(scancode);
}

/** KeyCode를 SDL_Scancode로 변환합니다. */
[[nodiscard]] constexpr SDL_Scancode ToSDLScancode(EKeyCode key) noexcept
{
    return static_cast<SDL_Scancode>(key);
}
} // namespace se
