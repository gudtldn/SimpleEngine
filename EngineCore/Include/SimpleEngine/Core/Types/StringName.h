#pragma once
#include <string_view>

#include "SimpleEngine/Core/Containers/Containers.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"


/**
 * 빠른 비교 및 조회를 위해 문자열을 ID로 관리하는 클래스
 */
class SE_CORE_API StringName
{
public:
    constexpr static size_t MAX_LENGTH = 256;
    static StringName None;

    /**
     * Pool에서 문자열을 찾습니다.
     * @param in_str 찾으려는 문자열
     * @return Pool에 존재하면 StringName값을 반환, 존재하지 않으면 StringName::None을 반환
     */
    [[nodiscard]] static StringName Find(const char8* in_str);

    /**
     * Pool에서 문자열을 찾습니다.
     * @param in_str 찾으려는 문자열
     * @return Pool에 존재하면 StringName값을 반환, 존재하지 않으면 StringName::None을 반환
     */
    [[nodiscard]] static StringName Find(std::u8string_view in_str);

public:
    StringName() = default;
    StringName(const char8* in_str);
    StringName(std::u8string_view in_str);

    [[nodiscard]] se::u8string ToString() const;
    [[nodiscard]] FORCE_INLINE uint64 GetDisplayHash() const { return display_hash; }
    [[nodiscard]] FORCE_INLINE uint64 GetComparisonHash() const { return comparison_hash; }

public:
    [[nodiscard]] FORCE_INLINE bool operator==(const StringName& other) const { return comparison_hash == other.comparison_hash; }
    [[nodiscard]] FORCE_INLINE bool operator!=(const StringName& other) const { return comparison_hash != other.comparison_hash; }

private:
    uint64 display_hash = 0;
    uint64 comparison_hash = 0;

#ifdef SE_DEBUG_BUILD
    const void* debug_entry_ptr = nullptr;
#endif
};


template <>
struct std::hash<StringName>
{
    uint64 operator()(const StringName& key) const noexcept
    {
        return hash<uint64>()(key.GetComparisonHash());
    }
};
