#pragma once

#include "SimpleEngine/Core/Container/FixedArray.h"
#include "SimpleEngine/Core/Container/StringView.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Core/Reflection/CanonicalName.h"

#if SE_HAS_REFLECTION
#include "SimpleEngine/Core/Reflection/CanonicalName_Reflect.h"
#else
#include "SimpleEngine/Core/Reflection/CanonicalName_FuncSig.h"
#endif


namespace se::detail
{
template <typename T>
consteval usize CanonicalNameLengthOf()
{
    return CanonicalNameOf<T>().size();
}

/**
 * canonical name을 null-terminated 정적 배열로 구워둡니다.
 * consteval 안에서만 살 수 있는 std::string을 런타임까지 끌어오기 위한 기법입니다.
 * 타입당 한 번만 인스턴스화되므로, 실제로 쓰인 타입의 이름만 .rodata에 들어갑니다.
 */
template <typename T>
constexpr auto CANONICAL_NAME_BUFFER = [] consteval
{
    constexpr usize LENGTH = CanonicalNameLengthOf<T>();

    FixedArray<char, LENGTH + 1> buffer{};
    const std::string name = CanonicalNameOf<T>();

    for (usize i = 0; i < LENGTH; ++i)
    {
        buffer[i] = name[i];
    }
    buffer[LENGTH] = '\0';

    return buffer;
}();
} // namespace se::detail

namespace se
{
/**
 * 타입 T의 정규화된 이름을 가져옵니다.
 * @tparam T 대상 타입
 * @return null-terminated canonical name
 */
template <typename T>
[[nodiscard]] constexpr StringView TypeNameOf() noexcept
{
    return {
        detail::CANONICAL_NAME_BUFFER<T>.Data(),
        detail::CANONICAL_NAME_BUFFER<T>.Len() - 1,
    };
}
} // namespace se
