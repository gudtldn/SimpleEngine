// ReSharper disable CppNonExplicitConvertingConstructor
#pragma once
#include <format>

#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/Container/StringView.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"


namespace se
{
/**
 * 빠른 비교 및 조회를 위해 문자열을 ID로 관리하는 클래스
 * @note ASCII 문자에 한해 대소문자를 구분하지 않습니다. (Unicode 문자는 대소문자 구분)
 */
class SE_CORE_API StringName
{
public:
    static StringName None;

    /**
     * Pool에서 문자열을 찾습니다.
     * @param in_str 찾으려는 문자열
     * @return Pool에 존재하면 StringName값을 반환, 존재하지 않으면 StringName::None을 반환
     */
    [[nodiscard]] static StringName Find(const char* in_str);

    /**
     * Pool에서 문자열을 찾습니다.
     * @param in_str 찾으려는 문자열
     * @return Pool에 존재하면 StringName값을 반환, 존재하지 않으면 StringName::None을 반환
     */
    [[nodiscard]] static StringName Find(StringView in_str);

public:
    StringName() = default;
    StringName(const char* in_str);
    StringName(const String& in_str);
    StringName(StringView in_str);

    [[nodiscard]] const char* CStr() const;
    [[nodiscard]] String ToString() const;
    [[nodiscard]] FORCE_INLINE uint64 GetComparisonHash() const { return comparison_hash; }

public:
    [[nodiscard]] FORCE_INLINE bool operator==(const StringName& other) const { return comparison_hash == other.comparison_hash; }
    [[nodiscard]] FORCE_INLINE bool operator!=(const StringName& other) const { return comparison_hash != other.comparison_hash; }

private:
    uint64 comparison_hash = 0;
    const char* display_name = nullptr;
};
}  // namespace se

template <>
struct std::hash<se::StringName>
{
    size_t operator()(const se::StringName& name) const noexcept
    {
        return hash<uint64>()(name.GetComparisonHash());
    }
};

// StringName에 대한 std::formatter 특수화
template <>
struct std::formatter<se::StringName, char> : std::formatter<se::StringView>
{
    auto format(const se::StringName& name, std::format_context& ctx) const
    {
        return std::formatter<se::StringView>::format(name.CStr(), ctx);
    }
};
