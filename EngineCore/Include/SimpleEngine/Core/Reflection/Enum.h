// ReSharper disable CppDFAUnreachableFunctionCall
#pragma once
#include <ranges>
#include <utility>

#include "SimpleEngine/Core/Container/FixedArray.h"
#include "SimpleEngine/Core/Container/Optional.h"
#include "SimpleEngine/Core/Container/StringView.h"
#include "SimpleEngine/Traits/TypeTraits.h"

/**
 * 일반 Enum의 탐색 범위를 확장합니다.
 * 예: SE_ENUM_RANGE(MyEnum, 0, 500);
 */
#define SE_ENUM_SET_RANGE(enum_type, min_value, max_value) \
    template <> struct se::detail::EnumTraits<enum_type> \
    { \
        static constexpr bool IsBitFlag = false; \
        static constexpr bool UseExplicitValues = false; \
        static constexpr int32 Min = static_cast<int32>(min_value); \
        static constexpr int32 Max = static_cast<int32>(max_value); \
    };

/**
 * 비트 플래그 Enum으로 지정합니다. (0~63 비트 자동 탐색)
 * 예: SE_ENUM_BITFLAG(MyFlags);
 */
#define SE_ENUM_SET_BITFLAG(enum_type) \
    template <> struct se::detail::EnumTraits<enum_type> \
    { \
        static constexpr bool IsBitFlag = true; \
        static constexpr bool UseExplicitValues = false; \
        static constexpr int32 Min = 0; \
        static constexpr int32 Max = 0; \
    };

/**
 * Enum을 지정한 값 목록으로 탐색합니다.
 * 예: SE_ENUM_SET_VALUES(1, 2, 3, ...);
 */
#define SE_ENUM_SET_VALUES(enum_type, ...) \
    template <> struct se::detail::EnumTraits<enum_type> \
    { \
        static constexpr bool IsBitFlag = false; \
        static constexpr bool UseExplicitValues = true; \
        static constexpr int32 Min = 0; \
        static constexpr int32 Max = 0; \
    }; \
    template <> struct se::detail::EnumExplicitValues<enum_type> \
    { \
        template <auto... Vs> \
        static consteval usize SizeOf() { return sizeof...(Vs); }\
        static constexpr FixedArray<enum_type, SizeOf<__VA_ARGS__>()> Values = { __VA_ARGS__ }; \
    };

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

    // constexpr StringView prefix_marker = "GetRawEnumSignature<";
    // auto start_param = signature.Find(prefix_marker);
    // if (!start_param.HasValue())
    // {
    //     return {};
    // }
    //
    // // prefix_marker 앞에서 시작
    // const usize search_start = *start_param + prefix_marker.ByteLen();
    //
    // constexpr StringView suffix_marker = ">(void)";
    // auto end_param = signature.FindLast(suffix_marker);
    // if (!end_param.HasValue())
    // {
    //     return {};
    // }
    //
    // // "< >" 사이의 문자열("enum MyEnum,MyEnum::Value" or "enum MyEnum,0") 추출
    // const StringView content = signature.Substr(search_start, *end_param - search_start);

    // 템플릿 인자 구분자 찾기 (Enum 이름 시작 위치)
    constexpr StringView prefix_marker = ",";
    auto start_param = signature.FindLast(prefix_marker);
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

    // ",...>(void)" 사이의 문자열 추출
    const StringView value_part = signature.Substr(search_start, *end_param - search_start);
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

/** Enum 리플렉션 동작 방식을 정의하는 구조체 */
template <traits::EnumType E>
struct EnumTraits
{
    static constexpr bool IsBitFlag = false;
    static constexpr bool UseExplicitValues = false;
    static constexpr int32 Min = 0;
    static constexpr int32 Max = 64;
};

/** 탐색할 리스트를 지정하는 구조체 */
template <typename E>
struct EnumExplicitValues;

/** 단일 Enum 항목 정보 */
template <typename E>
struct EnumEntry
{
    E value;
    StringView name;

    constexpr auto operator<=>(const EnumEntry& other) const { return value <=> other.value; }
};

/** Enum 값 생성기 */
template <typename E, bool IsBitFlag, bool UseExplicitValues>
struct EnumValueGenerator;

/** 일반 범위 (Min ~ Max) */
template <typename E>
struct EnumValueGenerator<E, false, false>
{
    static constexpr int32 Min = EnumTraits<E>::Min;
    static constexpr int32 Max = EnumTraits<E>::Max;
    static constexpr usize RangeSize = (Max - Min) + 1;

    // Sequence: 0, 1, 2 ...RangeSize
    using IndexSequence = std::make_integer_sequence<int32, RangeSize>;
    static constexpr E GetValue(int32 index) { return static_cast<E>(index + Min); }
};

/** 비트 플래그 (1<<0 ~ 1<<63) */
template <typename E>
struct EnumValueGenerator<E, true, false>
{
    // Sequence: 0, 1, 2 ... 63
    using IndexSequence = std::make_integer_sequence<int32, 64>;
    static constexpr E GetValue(int32 index) { return static_cast<E>(1ULL << index); }
};

template <typename E>
struct EnumValueGenerator<E, false, true>
{
    static constexpr auto& Values = EnumExplicitValues<E>::Values;
    static constexpr usize Count = Values.Len();

    using IndexSequence = std::make_integer_sequence<int32, Count>;
    static constexpr E GetValue(int32 index) { return Values[index]; }
};

/** 실제 데이터를 저장하는 구조체 */
template <typename E, typename Generator, typename Indices>
struct EnumStorageImpl;

template <typename E, typename Generator, int32... I>
struct EnumStorageImpl<E, Generator, std::integer_sequence<int32, I...>>
{
private:
    template <int32 Idx>
    static consteval EnumEntry<E> GetEntryIfValid()
    {
        constexpr E val = Generator::GetValue(Idx);
        constexpr StringView name = ExtractEnumName<E, val>();
        return EnumEntry<E>{ .value = val, .name = name };
    }

    static constexpr FixedArray<EnumEntry<E>, sizeof...(I)> RawEntries = { GetEntryIfValid<I>()... };

public:
    /** 유효한 Enum 개수 */
    static constexpr usize Count = std::ranges::count_if(RawEntries, [](const EnumEntry<E>& entry) -> bool
    {
        return !entry.name.IsEmpty();
    });

    /** 각 Entry를 모아서 배열 생성 */
    static constexpr FixedArray<EnumEntry<E>, Count> Entries = []
    {
        FixedArray<EnumEntry<E>, Count> entries{};
        std::ranges::copy_if(
            RawEntries, entries.begin(), [](const EnumEntry<E>& entry) -> bool
            {
                return !entry.name.IsEmpty();
            }
        );
        return entries;
    }();
};

/** Enum 정보를 저장하는 리플렉터 */
template <typename E>
struct EnumReflector
{
    static constexpr bool IsBitFlag = EnumTraits<E>::IsBitFlag;
    static constexpr bool UseExplicit = EnumTraits<E>::UseExplicitValues;

    using Generator = EnumValueGenerator<E, IsBitFlag, UseExplicit>;
    using Storage = EnumStorageImpl<E, Generator, typename Generator::IndexSequence>;

    static constexpr auto& Entries = Storage::Entries;
    static constexpr usize Count = Storage::Count;
};
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
    using Reflector = detail::EnumReflector<E>;
    constexpr auto& entries = detail::EnumReflector<E>::Entries;

    if constexpr (Reflector::UseExplicit)
    {
        for (const auto& entry : entries)
        {
            if (entry.value == value)
            {
                return entry.name;
            }
        }
    }
    else
    {
        auto it = std::lower_bound(entries.begin(), entries.end(), value, [](const auto& entry, E val)
        {
            return entry.value < val;
        });

        if (it != entries.end() && it->value == value)
        {
            return it->name;
        }
    }
    return {};
}

/** 문자열에 해당하는 열거형 값을 반환합니다. */
template <traits::EnumType E>
[[nodiscard]] constexpr Optional<E> EnumCast(StringView name) noexcept
{
    constexpr auto& entries = detail::EnumReflector<E>::Entries;
    for (const auto& entry : entries)
    {
        if (entry.name == name)
        {
            return entry.value;
        }
    }
    return {};
}
/** 열거형의 유효한 모든 항목을 반환합니다. */
template <traits::EnumType E>
[[nodiscard]] constexpr const auto& EnumEntries() noexcept
{
    return detail::EnumReflector<E>::Entries;
}

/** 열거형의 유효한 모든 값을 반환합니다. */
template <traits::EnumType E>
[[nodiscard]] constexpr const auto& EnumValues() noexcept
{
    using Reflector = detail::EnumReflector<E>;
    static constexpr auto values = []
    {
        FixedArray<E, Reflector::Count> ret{};
        std::ranges::copy(
            Reflector::Entries | std::views::transform([](const auto& entry) { return entry.value; }),
            ret.begin()
        );

        return ret;
    }();
    return values;
}

/** 열거형의 유효한 모든 이름들을 반환합니다. */
template <traits::EnumType E>
[[nodiscard]] constexpr const auto& EnumNames() noexcept
{
    using Reflector = detail::EnumReflector<E>;
    static constexpr auto names = []
    {
        FixedArray<StringView, Reflector::Count> ret{};
        std::ranges::copy(
            Reflector::Entries | std::views::transform([](const auto& entry) { return entry.name; }),
            ret.begin()
        );

        return ret;
    }();
    return names;
}

/** 열거형의 유효한 값 개수를 반환합니다. */
template <traits::EnumType E>
[[nodiscard]] consteval usize EnumCount() noexcept
{
    return detail::EnumReflector<E>::Count;
}
} // namespace se
