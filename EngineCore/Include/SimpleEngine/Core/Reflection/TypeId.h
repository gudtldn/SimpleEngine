#pragma once

#include "SimpleEngine/Core/Container/StringView.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Core/Reflection/CanonicalName.h"
#include "SimpleEngine/Utility/HashUtils.h"

#include <compare>
#include <functional>
#include <string>
#include <type_traits>


namespace se::detail
{
/** canonical name 인코딩 규칙의 버전 */
inline constexpr u64 TYPE_ID_SCHEMA_SALT = 1;

/**
 * name을 TypeId 원시 값으로 계산합니다.
 * @param name 타입 이름
 * @return 0이 아닌 64비트 해시 값
 */
[[nodiscard]] constexpr u64 ComputeTypeIdValue(StringView name) noexcept
{
    const u64 hash = hash::FNVWithSalt(name, TYPE_ID_SCHEMA_SALT);
    return hash == 0 ? u64{ 1 } : hash; // 0은 null 타입 전용으로 사용하기 때문에, 1로 대체
}

/**
 * 타입 T의 TypeId 원시 값을 계산합니다.
 * @tparam T 대상 타입
 * @return 0이 아닌 64비트 해시 값
 */
template <typename T>
consteval u64 ComputeTypeIdValue()
{
    return ComputeTypeIdValue(StringView{ CanonicalNameOf<T>() });
}

/** 각 타입별로 FNV 해시를 컴파일 타임에 계산해 캐싱합니다. */
template <typename T>
inline constexpr u64 TYPE_ID_VALUE = ComputeTypeIdValue<T>();
} // namespace se::detail

namespace se
{
/**
 * 타입의 컴파일 타임 분류와 런타임 판별만을 담당하는 8바이트 값 타입
 */
class TypeId final
{
public:
    /** null TypeId를 생성합니다. */
    constexpr TypeId() noexcept = default;

    /**
     * cvref를 제거한 clean type의 TypeId를 가져옵니다.
     * @tparam T 대상 타입
     * @return 해당 타입의 TypeId
     */
    template <typename T>
    [[nodiscard]] static consteval TypeId Of() noexcept
    {
        return TypeId{ detail::TYPE_ID_VALUE<std::remove_cvref_t<T>> };
    }

    /**
     * cv 한정자와 참조까지 구분하는 정확한 타입 기준의 TypeId를 가져옵니다.
     * @tparam T 대상 타입
     * @return 해당 타입의 TypeId
     */
    template <typename T>
    [[nodiscard]] static consteval TypeId OfExact() noexcept
    {
        return TypeId{ detail::TYPE_ID_VALUE<T> };
    }

    /**
     * 원시 값으로부터 TypeId를 복원합니다.
     * @param raw 원시 해시 값
     * @return 복원된 TypeId
     */
    [[nodiscard]] static constexpr TypeId FromRaw(u64 raw) noexcept
    {
        return TypeId{ raw };
    }

    /**
     * 정규 이름으로 TypeId를 구합니다.
     * @note Registry의 등록 여부와는 별개로 생성됩니다.
     */
    [[nodiscard]] static constexpr TypeId FromCanonicalName(StringView name) noexcept
    {
        return FromRaw(detail::ComputeTypeIdValue(name));
    }

    /** 타입 해시를 반환합니다. */
    [[nodiscard]] constexpr u64 Value() const noexcept { return hash; }

    /** null 여부를 반환합니다. */
    [[nodiscard]] constexpr bool IsNull() const noexcept { return hash == 0; }

    constexpr explicit operator bool() const noexcept { return hash != 0; }
    friend constexpr bool operator==(TypeId, TypeId) noexcept = default;
    friend constexpr std::strong_ordering operator<=>(TypeId, TypeId) noexcept = default;

private:
    constexpr explicit TypeId(u64 value) noexcept
        : hash(value)
    {
    }

    u64 hash = 0;
};

static_assert(sizeof(TypeId) == 8);
static_assert(std::is_trivially_copyable_v<TypeId>);
static_assert(TypeId{}.IsNull());
static_assert(!static_cast<bool>(TypeId{}));
} // namespace se

template <>
struct std::hash<se::TypeId>
{
    [[nodiscard]] constexpr usize operator()(se::TypeId id) const noexcept
    {
        return static_cast<usize>(id.Value());
    }
};
