#pragma once
#include <utility>

#include "SimpleEngine/Core/Container/FixedArray.h"
#include "SimpleEngine/Core/Container/Optional.h"
#include "SimpleEngine/Core/Container/StringView.h"
#include "SimpleEngine/Traits/TypeTraits.h"


namespace se
{
namespace detail
{
/** 문자가 유효한 식별자인지 확인합니다. */
[[nodiscard]] consteval bool IsValidIdentifierStart(char c) noexcept
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

/**
 * 컴파일러에 종속적인 Enum 값 시그니처를 반환합니다.
 * @return 함수 시그니처 문자열
 */
template <typename EnumType, EnumType Value>
consteval StringView GetRawEnumSignature() noexcept
{
#if SE_COMPILER_MSVC
    return __FUNCSIG__;
#elif SE_COMPILER_CLANG || SE_COMPILER_GCC
    return __PRETTY_FUNCTION__;
#else
    #error "Unsupported compiler for enum reflection"
#endif
}

/** MSVC 시그니처에서 Enum 이름을 추출합니다. */
consteval StringView ExtractEnumName_MSVC(StringView signature) noexcept
{
    // MSVC Example:
    // "... GetRawEnumSignature<enum EnumType,EnumType::Value>(void) ..."
    // "... GetRawEnumSignature<enum EnumType,0>(void) ..."

    constexpr StringView prefix_marker = "GetRawEnumSignature<";
    auto start_param = signature.Find(prefix_marker);
    if (!start_param.HasValue())
    {
        return {};
    }

    // prefix_marker 앞에서 시작
    const usize search_start = *start_param + prefix_marker.ByteLen();

    constexpr StringView suffix_marker = ">(void)";
    auto end_param = signature.FindLast(suffix_marker);
    if (!end_param.HasValue())
    {
        return {};
    }

    // "< >" 사이의 문자열("enum MyEnum,MyEnum::Value" or "enum MyEnum,0") 추출
    const StringView content = signature.Substr(search_start, *end_param - search_start);

    // 템플릿 인자 구분자 찾기
    auto last_comma = content.FindLast(",");
    if (!last_comma.HasValue())
    {
        return {};
    }

    const StringView value_part = content.Substr(*last_comma + 1).Trim();
    if (value_part.IsEmpty())
    {
        return {};
    }

    // value_part 유효성 검사 (숫자로 시작하면 이름 없는 값)
    if (!IsValidIdentifierStart(value_part[0]))
    {
        return {};
    }

    // Scope 제거 (MyEnum::Value -> Value)
    if (const auto last_scope = value_part.FindLast("::"))
    {
        return value_part.Substr(*last_scope + 2);
    }
    return value_part;
}

/** GCC/Clang 시그니처에서 Enum 이름을 추출합니다. */
consteval StringView ExtractEnumName_GCC_Clang(StringView signature) noexcept
{
    // GCC Example: "... [with EnumType = MyEnum; EnumType Value = MyEnum::A; ...]"
    // Clang Example: "... [EnumType = MyEnum, Value = MyEnum::A]"

    constexpr StringView prefix_marker = "Value = ";
    const auto start_param = signature.Find(prefix_marker);
    if (!start_param.HasValue())
    {
        return {};
    }
    const usize search_start = *start_param + prefix_marker.ByteLen();

#if SE_COMPILER_GCC
    constexpr StringView suffix_marker = ";";
    const auto end_param = signature.Find(suffix_marker);
#else
    constexpr StringView suffix_marker = "]";
    const auto end_param = signature.FindLast(suffix_marker);
#endif
    if (!end_param.HasValue())
    {
        return {};
    }

    const StringView value_part = signature.Substr(search_start, *end_param - search_start).Trim();
    if (value_part.IsEmpty())
    {
        return {};
    }

    // value_part 유효성 검사
    if (!IsValidIdentifierStart(value_part[0]))
    {
        return {};
    }

    // Scope 제거 (MyEnum::Value -> Value)
    if (const auto last_scope = value_part.FindLast("::"))
    {
        return value_part.Substr(*last_scope + 2);
    }
    return value_part;
}

/** 컴파일 타임에 Enum::Value의 이름을 추출합니다. */
template <typename EnumType, EnumType Value>
consteval StringView ExtractEnumName() noexcept
{
    constexpr auto signature = GetRawEnumSignature<EnumType, Value>();

#if SE_COMPILER_MSVC
    return ExtractEnumName_MSVC(signature);
#elif SE_COMPILER_CLANG || SE_COMPILER_GCC
    return ExtractEnumName_GCC_Clang(signature);
#else
    #error "Unsupported compiler for type name extraction"
#endif
}

/** 주어진 Enum 값이 유효한지(이름이 있는지) 확인합니다. */
template <typename EnumType, EnumType Value>
consteval bool IsValidEnum() noexcept
{
    constexpr StringView name = ExtractEnumName<EnumType, Value>();
    return !name.IsEmpty();
}

// 자동 Enum 탐색을 위한 기본 범위
constexpr int32 ENUM_MIN = -128;
constexpr int32 ENUM_MAX = 127;

template <typename E, typename Seq>
struct EnumStorage;

template <typename E, int32... I>
struct EnumStorage<E, std::integer_sequence<int32, I...>>
{
    using UnderlyingType = std::underlying_type_t<E>;

    /** 인덱스를 실제 Enum 값으로 변환합니다. */
    static constexpr E IndexToEnum(int32 index)
    {
        return static_cast<E>(static_cast<UnderlyingType>(index + ENUM_MIN));
    }

public:
    static constexpr usize Count = ((IsValidEnum<E, IndexToEnum(I)>() ? 1 : 0) + ...);

    static constexpr FixedArray<E, Count> Values = []
    {
        FixedArray<E, Count> values{};
        usize idx = 0;
        ((IsValidEnum<E, IndexToEnum(I)>() ? (values[idx++] = IndexToEnum(I), 0) : 0), ...);
        return values;
    }();

    static constexpr FixedArray<StringView, Count> Names = []
    {
        FixedArray<StringView, Count> names{};
        usize idx = 0;
        ((IsValidEnum<E, IndexToEnum(I)>() ? (names[idx++] = ExtractEnumName<E, IndexToEnum(I)>(), 0) : 0), ...);
        return names;
    }();
};

/** Enum 정보를 저장하는 리플렉터 */
template <typename E>
using EnumReflector = EnumStorage<E, std::make_integer_sequence<int32, ENUM_MAX - ENUM_MIN + 1>>;
} // namespace detail

/**
 * 열거형 값에 해당하는 문자열 이름을 반환합니다.
 * 유효하지 않은 값일 경우 빈 문자열을 반환합니다.
 */
template <auto V>
    requires traits::EnumType<decltype(V)>
[[nodiscard]] consteval StringView EnumName() noexcept
{
    using E = decltype(V);
    return detail::ExtractEnumName<E, V>();
}

/**
 * 열거형 값에 해당하는 문자열 이름을 반환합니다.
 * 값이 유효 범위 내에 있고 이름이 있다면 반환, 아니면 빈 문자열을 반환합니다.
 */
template <traits::EnumType E>
[[nodiscard]] constexpr StringView EnumName(E value) noexcept
{
    constexpr auto& values = detail::EnumReflector<E>::Values;
    constexpr auto& names = detail::EnumReflector<E>::Names;

    for (usize i = 0; i < values.Len(); ++i)
    {
        if (values[i] == value)
        {
            return names[i];
        }
    }
    return {};
}

/** 문자열에 해당하는 열거형 값을 반환합니다. */
template <traits::EnumType E>
[[nodiscard]] constexpr Optional<E> EnumCast(StringView name) noexcept
{
    constexpr auto& values = detail::EnumReflector<E>::Values;
    constexpr auto& names = detail::EnumReflector<E>::Names;

    for (usize i = 0; i < names.Len(); ++i)
    {
        if (names[i] == name)
        {
            return values[i];
        }
    }
    return {};
}

/** 열거형의 유효한 모든 값을 반환합니다. */
template <traits::EnumType E>
[[nodiscard]] constexpr const auto& EnumValues() noexcept
{
    return detail::EnumReflector<E>::Values;
}

/** 열거형의 유효한 모든 이름들을 반환합니다. */
template <traits::EnumType E>
[[nodiscard]] constexpr const auto& EnumNames() noexcept
{
    return detail::EnumReflector<E>::Names;
}

/** 열거형의 유효한 값 개수를 반환합니다. */
template <traits::EnumType E>
[[nodiscard]] consteval usize EnumCount() noexcept
{
    return detail::EnumReflector<E>::Count;
}
} // namespace se
