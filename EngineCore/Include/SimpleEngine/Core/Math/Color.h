// NOLINTBEGIN(*-use-std-numbers)

#pragma once

#include "SimpleEngine/Core/Container/FixedArray.h"
#include "SimpleEngine/Core/Math/MathUtility.h"


namespace se::math
{
// forward declarations
struct Color;

namespace detail
{
/** Linear -> sRGB 변환 */
constexpr float LinearToSrgb(float val)
{
    if (val <= 0.0031308f)
    {
        return val * 12.92f;
    }
    return (1.055f * Pow(val, 1.0f / 2.4f)) - 0.055f;
}

/** sRGB -> Linear Lookup Table */
constexpr float SRGB_TO_LINEAR_LUT[256] = {
    0.000000f, 0.000304f, 0.000607f, 0.000911f, 0.001214f, 0.001518f, 0.001821f, 0.002125f,
    0.002428f, 0.002732f, 0.003035f, 0.003347f, 0.003677f, 0.004025f, 0.004391f, 0.004777f,
    0.005182f, 0.005605f, 0.006049f, 0.006512f, 0.006995f, 0.007499f, 0.008023f, 0.008568f,
    0.009134f, 0.009721f, 0.010330f, 0.010960f, 0.011612f, 0.012286f, 0.012983f, 0.013702f,
    0.014444f, 0.015209f, 0.015996f, 0.016807f, 0.017642f, 0.018500f, 0.019382f, 0.020289f,
    0.021219f, 0.022174f, 0.023153f, 0.024158f, 0.025187f, 0.026241f, 0.027321f, 0.028426f,
    0.029557f, 0.030713f, 0.031896f, 0.033105f, 0.034340f, 0.035601f, 0.036889f, 0.038204f,
    0.039546f, 0.040915f, 0.042311f, 0.043735f, 0.045186f, 0.046665f, 0.048172f, 0.049707f,
    0.051269f, 0.052861f, 0.054480f, 0.056128f, 0.057805f, 0.059511f, 0.061246f, 0.063010f,
    0.064803f, 0.066626f, 0.068478f, 0.070360f, 0.072272f, 0.074214f, 0.076185f, 0.078187f,
    0.080220f, 0.082283f, 0.084376f, 0.086500f, 0.088656f, 0.090842f, 0.093059f, 0.095307f,
    0.097587f, 0.099899f, 0.102242f, 0.104616f, 0.107023f, 0.109462f, 0.111932f, 0.114435f,
    0.116971f, 0.119538f, 0.122139f, 0.124772f, 0.127438f, 0.130136f, 0.132868f, 0.135633f,
    0.138432f, 0.141263f, 0.144128f, 0.147027f, 0.149960f, 0.152926f, 0.155926f, 0.158961f,
    0.162029f, 0.165132f, 0.168269f, 0.171441f, 0.174647f, 0.177888f, 0.181164f, 0.184475f,
    0.187821f, 0.191202f, 0.194618f, 0.198069f, 0.201556f, 0.205079f, 0.208637f, 0.212231f,
    0.215861f, 0.219526f, 0.223228f, 0.226966f, 0.230740f, 0.234551f, 0.238398f, 0.242281f,
    0.246201f, 0.250158f, 0.254152f, 0.258183f, 0.262251f, 0.266356f, 0.270498f, 0.274677f,
    0.278894f, 0.283149f, 0.287441f, 0.291771f, 0.296138f, 0.300544f, 0.304987f, 0.309469f,
    0.313989f, 0.318547f, 0.323143f, 0.327778f, 0.332452f, 0.337164f, 0.341914f, 0.346704f,
    0.351533f, 0.356400f, 0.361307f, 0.366253f, 0.371238f, 0.376262f, 0.381326f, 0.386429f,
    0.391572f, 0.396755f, 0.401978f, 0.407240f, 0.412543f, 0.417885f, 0.423268f, 0.428690f,
    0.434154f, 0.439657f, 0.445201f, 0.450786f, 0.456411f, 0.462077f, 0.467784f, 0.473531f,
    0.479320f, 0.485150f, 0.491021f, 0.496933f, 0.502886f, 0.508881f, 0.514918f, 0.520996f,
    0.527115f, 0.533276f, 0.539479f, 0.545724f, 0.552011f, 0.558340f, 0.564712f, 0.571125f,
    0.577580f, 0.584078f, 0.590619f, 0.597202f, 0.603827f, 0.610496f, 0.617207f, 0.623960f,
    0.630757f, 0.637597f, 0.644480f, 0.651406f, 0.658375f, 0.665387f, 0.672443f, 0.679542f,
    0.686685f, 0.693872f, 0.701102f, 0.708376f, 0.715694f, 0.723055f, 0.730461f, 0.737910f,
    0.745404f, 0.752942f, 0.760525f, 0.768151f, 0.775822f, 0.783538f, 0.791298f, 0.799103f,
    0.806952f, 0.814847f, 0.822786f, 0.830770f, 0.838799f, 0.846873f, 0.854993f, 0.863157f,
    0.871367f, 0.879622f, 0.887923f, 0.896269f, 0.904661f, 0.913099f, 0.921582f, 0.930111f,
    0.938686f, 0.947307f, 0.955973f, 0.964686f, 0.973445f, 0.982251f, 0.991102f, 1.000000f,
};
} // namespace detail

/**
 * float 기반 색상 구조체 (R, G, B, A)
 */
struct LinearColor
{
    float r, g, b, a;

public:
    constexpr LinearColor()
        : r(0.0f), g(0.0f), b(0.0f), a(1.0f)
    {
    }

    constexpr LinearColor(float in_r, float in_g, float in_b, float in_a = 1.0f)
        : r(in_r), g(in_g), b(in_b), a(in_a)
    {
    }

    /**
     * 8비트 Color를 LinearColor로 변환합니다.
     * @param color 원본 8비트 색상
     * @param is_srgb true면 sRGB -> Linear 변환 수행, false면 단순 정규화(/255)만 수행
     */
    explicit constexpr LinearColor(const Color& color, bool is_srgb = true);

    /** HSV 색상 공간으로부터 LinearRGB 색상을 생성합니다. */
    static constexpr LinearColor FromHSV(float hue, float saturation, float value, float alpha = 1.0f);

public:
    [[nodiscard]] static constexpr LinearColor Black()       { return { 0.0f, 0.0f, 0.0f, 1.0f }; }
    [[nodiscard]] static constexpr LinearColor White()       { return { 1.0f, 1.0f, 1.0f, 1.0f }; }
    [[nodiscard]] static constexpr LinearColor Red()         { return { 1.0f, 0.0f, 0.0f, 1.0f }; }
    [[nodiscard]] static constexpr LinearColor Green()       { return { 0.0f, 1.0f, 0.0f, 1.0f }; }
    [[nodiscard]] static constexpr LinearColor Blue()        { return { 0.0f, 0.0f, 1.0f, 1.0f }; }
    [[nodiscard]] static constexpr LinearColor Yellow()      { return { 1.0f, 1.0f, 0.0f, 1.0f }; }
    [[nodiscard]] static constexpr LinearColor Cyan()        { return { 0.0f, 1.0f, 1.0f, 1.0f }; }
    [[nodiscard]] static constexpr LinearColor Magenta()     { return { 1.0f, 0.0f, 1.0f, 1.0f }; }
    [[nodiscard]] static constexpr LinearColor Transparent() { return { 0.0f, 0.0f, 0.0f, 0.0f }; }

public:
    /** 값을 min~max 범위로 자릅니다. (Saturate 등) */
    constexpr void Clamp(float min_value = 0.0f, float max_value = 1.0f)
    {
        r = math::Clamp(r, min_value, max_value);
        g = math::Clamp(g, min_value, max_value);
        b = math::Clamp(b, min_value, max_value);
        a = math::Clamp(a, min_value, max_value);
    }

    /** Clamp된 복사본을 반환합니다. */
    [[nodiscard]] constexpr LinearColor GetClamped(float min_value = 0.0f, float max_value = 1.0f) const
    {
        LinearColor result = *this;
        result.Clamp(min_value, max_value);
        return result;
    }

    /** 현재 RGB 색상을 HSV 모델로 변환하여 반환합니다. */
    [[nodiscard]] constexpr LinearColor LinearRGBToHSV() const;

    /** 현재 값을 HSV로 해석하여 다시 RGB로 변환합니다. (r=H, g=S, b=V라 가정) */
    [[nodiscard]] constexpr LinearColor HSVToLinearRGB() const;

    /**
     * LinearColor를 8비트 Color로 변환합니다. (모니터 출력/저장용)
     * * 연산이 끝난 선형 색상을 사람이 보기 좋게 만들기 위해 다시 감마 인코딩(Linear->sRGB)을 수행합니다.
     */
    [[nodiscard]] constexpr Color ToColor(bool is_srgb = true) const;

    /** 두 색상이 오차 범위 내에서 같은지 비교합니다. (부동소수점 오차 고려) */
    [[nodiscard]] constexpr bool IsNearlyEqual(const LinearColor& other, float tolerance = KINDA_SMALL_NUMBER) const
    {
        return Abs(r - other.r) <= tolerance
            && Abs(g - other.g) <= tolerance
            && Abs(b - other.b) <= tolerance
            && Abs(a - other.a) <= tolerance;
    }

public:
    [[nodiscard]] constexpr LinearColor operator+(const LinearColor& other) const
    {
        return { r + other.r, g + other.g, b + other.b, a + other.a };
    }

    FORCE_INLINE LinearColor& operator+=(const LinearColor& other)
    {
        r += other.r; g += other.g; b += other.b; a += other.a;
        return *this;
    }

    [[nodiscard]] constexpr LinearColor operator-(const LinearColor& other) const
    {
        return { r - other.r, g - other.g, b - other.b, a - other.a };
    }

    FORCE_INLINE LinearColor& operator-=(const LinearColor& other)
    {
        r -= other.r; g -= other.g; b -= other.b; a -= other.a;
        return *this;
    }

    [[nodiscard]] constexpr LinearColor operator*(const LinearColor& other) const
    {
        return { r * other.r, g * other.g, b * other.b, a * other.a };
    }

    FORCE_INLINE LinearColor& operator*=(const LinearColor& other)
    {
        r *= other.r; g *= other.g; b *= other.b; a *= other.a;
        return *this;
    }

    [[nodiscard]] constexpr LinearColor operator*(float scalar) const
    {
        return { r * scalar, g * scalar, b * scalar, a * scalar };
    }

    FORCE_INLINE LinearColor& operator*=(float scalar)
    {
        r *= scalar; g *= scalar; b *= scalar; a *= scalar;
        return *this;
    }

    [[nodiscard]] constexpr LinearColor operator/(const LinearColor& other) const
    {
        return { r / other.r, g / other.g, b / other.b, a / other.a };
    }

    FORCE_INLINE LinearColor& operator/=(const LinearColor& other)
    {
        r /= other.r; g /= other.g; b /= other.b; a /= other.a;
        return *this;
    }

    [[nodiscard]] constexpr LinearColor operator/(float scalar) const
    {
        const float inv_scalar = 1.0f / scalar;
        return { r * inv_scalar, g * inv_scalar, b * inv_scalar, a * inv_scalar };
    }

    FORCE_INLINE LinearColor& operator/=(float scalar)
    {
        const float inv_scalar = 1.0f / scalar;
        r *= inv_scalar; g *= inv_scalar; b *= inv_scalar; a *= inv_scalar;
        return *this;
    }

    [[nodiscard]] constexpr bool operator==(const LinearColor& other) const = default;
};

/**
 * 8비트 정수 기반 색상 구조체 (R, G, B, A)
 */
struct Color
{
    uint8 r, g, b, a;

public:
    constexpr Color()
    {
        r = g = b = 0;
        a = 255;
    }

    constexpr Color(uint8 in_r, uint8 in_g, uint8 in_b, uint8 in_a = 255)
        : r(in_r), g(in_g), b(in_b), a(in_a)
    {
    }

    /**
     * 0xRRGGBBAA 형식의 정수로부터 색상을 생성합니다.
     * 내부적으로 Shift 연산을 사용하므로 엔디안과 무관하게 R,G,B,A 순서가 보장됩니다.
     */
    explicit constexpr Color(uint32 rgba)
        : r(static_cast<uint8>((rgba >> 24) & 0xFF))
        , g(static_cast<uint8>((rgba >> 16) & 0xFF))
        , b(static_cast<uint8>((rgba >> 8) & 0xFF))
        , a(static_cast<uint8>(rgba & 0xFF))
    {
    }

    /**
     * LinearColor를 8비트 Color로 변환하여 생성합니다.
     * @param linear_color 변환할 LinearColor
     * @param is_srgb true면 Linear -> sRGB 변환 수행, false면 단순 정규화(/1.0f)만 수행
     */
    explicit constexpr Color(const LinearColor& linear_color, bool is_srgb = true);

public:
    [[nodiscard]] static constexpr Color Black()       { return { 0,   0,   0   }; }
    [[nodiscard]] static constexpr Color White()       { return { 255, 255, 255 }; }
    [[nodiscard]] static constexpr Color Red()         { return { 255, 0,   0   }; }
    [[nodiscard]] static constexpr Color Green()       { return { 0,   255, 0   }; }
    [[nodiscard]] static constexpr Color Blue()        { return { 0,   0,   255 }; }
    [[nodiscard]] static constexpr Color Yellow()      { return { 255, 255, 0   }; }
    [[nodiscard]] static constexpr Color Cyan()        { return { 0,   255, 255 }; }
    [[nodiscard]] static constexpr Color Magenta()     { return { 255, 0,   255 }; }
    [[nodiscard]] static constexpr Color Transparent() { return { 0,  0,  0,  0 }; }

public:
    /** @return ARGB 포맷의 32비트 정수 */
    [[nodiscard]] constexpr uint32 ToPackedARGB() const
    {
        return (static_cast<uint32>(a) << 24) | (static_cast<uint32>(r) << 16)
            | (static_cast<uint32>(g) << 8) | (static_cast<uint32>(b) << 0);
    }

    /** @return RGBA 포맷의 32비트 정수 */
    [[nodiscard]] constexpr uint32 ToPackedRGBA() const
    {
        return (static_cast<uint32>(r) << 24) | (static_cast<uint32>(g) << 16)
            | (static_cast<uint32>(b) << 8) | (static_cast<uint32>(a) << 0);
    }

    /** LinearColor로 변환합니다. */
    [[nodiscard]] constexpr LinearColor ToLinearColor(bool is_srgb = true) const;

public:
    [[nodiscard]] constexpr bool operator==(const Color& other) const = default;
};

constexpr LinearColor::LinearColor(const Color& color, bool is_srgb)
{
    constexpr float inv = 1.0f / 255.0f;
    if (is_srgb)
    {
        r = detail::SRGB_TO_LINEAR_LUT[color.r];
        g = detail::SRGB_TO_LINEAR_LUT[color.g];
        b = detail::SRGB_TO_LINEAR_LUT[color.b];
        a = static_cast<float>(color.a) * inv;
    }
    else
    {
        r = static_cast<float>(color.r) * inv;
        g = static_cast<float>(color.g) * inv;
        b = static_cast<float>(color.b) * inv;
        a = static_cast<float>(color.a) * inv;
    }
}

constexpr LinearColor LinearColor::FromHSV(float hue, float saturation, float value, float alpha)
{
    const float c = value * saturation;
    float h_prime = Fmod(hue / 60.0f, 6.0f);
    if (h_prime < 0.0f)
    {
        h_prime += 6.0f; // Fmod result can be negative
    }

    const float x = c * (1.0f - Abs(Fmod(h_prime, 2.0f) - 1.0f));
    const float m = value - c;

    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;

    if      (h_prime < 1.0f) { r = c; g = x; b = 0; }
    else if (h_prime < 2.0f) { r = x; g = c; b = 0; }
    else if (h_prime < 3.0f) { r = 0; g = c; b = x; }
    else if (h_prime < 4.0f) { r = 0; g = x; b = c; }
    else if (h_prime < 5.0f) { r = x; g = 0; b = c; }
    else                     { r = c; g = 0; b = x; }

    return { r + m, g + m, b + m, alpha };
}

constexpr LinearColor LinearColor::LinearRGBToHSV() const
{
    const float max_v = Max(r, Max(g, b));
    const float min_v = Min(r, Min(g, b));
    const float delta = max_v - min_v;

    float h = 0.0f;
    float s = 0.0f;
    float v = max_v;

    if (delta > KINDA_SMALL_NUMBER)
    {
        s = delta / max_v;

        if (max_v == r)
        {
            h = (g - b) / delta;
            if (g < b) { h += 6.0f; }
        }
        else if (max_v == g)
        {
            h = ((b - r) / delta) + 2.0f;
        }
        else // max_v == b
        {
            h = ((r - g) / delta) + 4.0f;
        }

        h *= 60.0f;
    }

    return { h, s, v, a };
}

constexpr LinearColor LinearColor::HSVToLinearRGB() const
{
    // r=Hue, g=Saturation, b=Value
    return FromHSV(r, g, b, a);
}

constexpr Color LinearColor::ToColor(bool is_srgb) const
{
    LinearColor clamped = GetClamped();
    if (is_srgb)
    {
        clamped.r = detail::LinearToSrgb(clamped.r);
        clamped.g = detail::LinearToSrgb(clamped.g);
        clamped.b = detail::LinearToSrgb(clamped.b);
    }

    return {
        static_cast<uint8>(RoundToInt(clamped.r * 255.0f)),
        static_cast<uint8>(RoundToInt(clamped.g * 255.0f)),
        static_cast<uint8>(RoundToInt(clamped.b * 255.0f)),
        static_cast<uint8>(RoundToInt(clamped.a * 255.0f)),
    };
}

constexpr Color::Color(const LinearColor& linear_color, bool is_srgb)
{
    *this = linear_color.ToColor(is_srgb);
}

constexpr LinearColor Color::ToLinearColor(bool is_srgb) const
{
    return LinearColor(*this, is_srgb);
}
} // namespace se::math

// NOLINTEND(*-use-std-numbers)
