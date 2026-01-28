#pragma once
#include "SimpleEngine/Core/Container/FixedArray.h"
#include "SimpleEngine/Core/Math/MathUtility.h"


namespace se::math
{
struct Color;

namespace details
{
consteval FixedArray<float, 256> CreateSRGBToLinearTable()
{
    FixedArray<float, 256> table;
    for (int i = 0; i < 256; ++i)
    {
        const float val = static_cast<float>(i) / 255.0f;
        if (val <= 0.04045f)
        {
            table[i] = val / 12.92f;
        }
        else
        {
            table[i] = Pow((val + 0.055f) / 1.055f, 2.4f);
        }
    }
    return table;
}

/** sRGB -> Linear 변환 */
constexpr float SrgbToLinear(float val)
{
    if (val <= 0.04045f)
    {
        return val / 12.92f;
    }
    return Pow((val + 0.055f) / 1.055f, 2.4f);
}

/** Linear -> sRGB 변환 */
constexpr float LinearToSrgb(float val)
{
    if (val <= 0.0031308f)
    {
        return val * 12.92f;
    }
    return (1.055f * Pow(val, 1.0f / 2.4f)) - 0.055f;
}
}  // namespace details

/**
 * @todo docs
 */
struct LinearColor
{
    float r, g, b, a;

    static constexpr FixedArray<float, 256> SRGB_TO_LINEAR_TABLE = details::CreateSRGBToLinearTable();

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
     * @brief Color로부터 LinearColor를 생성합니다.
     * @param color 원본 Color
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
    /**
     *
     * @param min_value
     * @param max_value
     */
    constexpr void Clamp(float min_value = 0.0f, float max_value = 1.0f)
    {
        r = math::Clamp(r, min_value, max_value);
        g = math::Clamp(g, min_value, max_value);
        b = math::Clamp(b, min_value, max_value);
        a = math::Clamp(a, min_value, max_value);
    }

    /**
     *
     * @param min_value
     * @param max_value
     * @return
     */
    [[nodiscard]] constexpr LinearColor GetClamped(float min_value = 0.0f, float max_value = 1.0f) const
    {
        LinearColor result = *this;
        result.Clamp(min_value, max_value);
        return result;
    }

    /**
     *
     * @return
     */
    [[nodiscard]] constexpr LinearColor LinearRGBToHSV() const;

    /**
     *
     * @return
     */
    [[nodiscard]] constexpr LinearColor HSVToLinearRGB() const;

    /**
     *
     * @param is_srgb
     * @return
     */
    [[nodiscard]] constexpr Color ToColor(bool is_srgb = true) const;

    /**
     *
     * @param other
     * @param tolerance
     * @return
     */
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
 * @todo docs
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
        , a(static_cast<uint8>((rgba) & 0xFF))
    {
    }

    /**
     * @brief LinearColor를 Color로 변환하여 생성합니다.
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
    [[nodiscard]] constexpr uint32 ToPackedARGB() const
    {
        return (static_cast<uint32>(a) << 24) | (static_cast<uint32>(r) << 16)
             | (static_cast<uint32>(g) << 8)  | (static_cast<uint32>(b) << 0);
    }

    [[nodiscard]] constexpr uint32 ToPackedRGBA() const
    {
        return (static_cast<uint32>(r) << 24) | (static_cast<uint32>(g) << 16)
             | (static_cast<uint32>(b) << 8)  | (static_cast<uint32>(a) << 0);
    }

    [[nodiscard]] constexpr LinearColor ToLinearColor(bool is_srgb = true) const;

public:
    [[nodiscard]] constexpr bool operator==(const Color& other) const = default;
};

constexpr LinearColor::LinearColor(const Color& color, bool is_srgb)
{
    constexpr float inv = 1.0f / 255.0f;
    if (is_srgb)
    {
        r = SRGB_TO_LINEAR_TABLE[color.r];
        g = SRGB_TO_LINEAR_TABLE[color.g];
        b = SRGB_TO_LINEAR_TABLE[color.b];
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
        clamped.r = details::LinearToSrgb(clamped.r);
        clamped.g = details::LinearToSrgb(clamped.g);
        clamped.b = details::LinearToSrgb(clamped.b);
    }

    // NOLINTBEGIN(*-incorrect-roundings)
    return {
        static_cast<uint8>((clamped.r * 255.0f) + 0.5f),
        static_cast<uint8>((clamped.g * 255.0f) + 0.5f),
        static_cast<uint8>((clamped.b * 255.0f) + 0.5f),
        static_cast<uint8>((clamped.a * 255.0f) + 0.5f),
    };
    // NOLINTEND(*-incorrect-roundings)
}

constexpr Color::Color(const LinearColor& linear_color, bool is_srgb)
{
    *this = linear_color.ToColor(is_srgb);
}

constexpr LinearColor Color::ToLinearColor(bool is_srgb) const
{
    return LinearColor(*this, is_srgb);
}

}  // namespace se::math
