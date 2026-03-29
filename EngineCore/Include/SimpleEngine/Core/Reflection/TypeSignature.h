#pragma once
#include "SimpleEngine/Core/Container/FixedArray.h"
#include "SimpleEngine/Core/Container/StringView.h"
#include "SimpleEngine/Traits/TypeTraits.h"


namespace se
{
namespace detail
{
/** 들어온 값이 토큰 경계 문자인지 여부를 구합니다. */
[[nodiscard]] consteval bool IsTokenBoundary(char c) noexcept
{
    // C++ 타입 선언에서 토큰 경계로 취급할 수 있는 문자 집합
    return c == ' ' || c == '*'
        || c == '&' || c == ','
        || c == '<' || c == '>'
        || c == ':' || c == '\0';
}

/**
 * 들어온 시그니처에서 cv한정자 및 포인터, 참조를 제거합니다.
 *
 * @tparam N 한정자 배열의 크기
 * @param signature 원본 문자열
 * @param keywords 제거할 한정자의 배열
 * @return 한정자가 제거된 시그니처를 반환
 */
template <usize N>
consteval StringView RemoveKeywords(StringView signature, const FixedArray<StringView, N>& keywords) noexcept
{
    bool modified;
    do
    {
        modified = false;

        for (const StringView& keyword : keywords)
        {
            if (signature.StartsWith(keyword))
            {
                // keyword 다음 문자가 토큰 경계인지 확인
                const char next = signature.ByteLen() > keyword.ByteLen() ? signature[keyword.ByteLen()] : '\0';
                if (IsTokenBoundary(next))
                {
                    signature.RemovePrefix(keyword.ByteLen());
                    modified = true;
                    break;
                }
            }
        }

        if (modified)
        {
            signature = signature.Trim();
        }
    }
    while (modified);
    return signature;
}

/** signature에서 namespace를 제거한 원본 타입명을 반환합니다. */
// ReSharper disable once CppDFAUnreachableFunctionCall
consteval StringView RemoveNamespace(StringView signature) noexcept
{
    constexpr StringView ns_sep = "::";
    if (const auto pos_opt = signature.FindLast(ns_sep))
    {
        return signature.Substr(*pos_opt + ns_sep.ByteLen());
    }
    return signature;
}

template <typename T>
struct UnwrapTypeImpl { using Type = T; };

template <typename T>
struct UnwrapTypeImpl<T*> : UnwrapTypeImpl<std::remove_cv_t<T>> {};

template <typename T>
struct UnwrapTypeImpl<T&> : UnwrapTypeImpl<std::remove_cv_t<T>> {};

template <typename T>
struct UnwrapTypeImpl<T&&> : UnwrapTypeImpl<std::remove_cv_t<T>> {};

template <typename T>
struct UnwrapTypeImpl<T[]> : UnwrapTypeImpl<std::remove_cv_t<T>> {};

template <typename T, usize N>
struct UnwrapTypeImpl<T[N]> : UnwrapTypeImpl<std::remove_cv_t<T>> {};

/** 타입에서 모든 포인터, 배열, 참조, cv-한정자를 재귀적으로 제거하여 가장 근본이 되는 기본 타입을 추출합니다. */
template <typename T>
using UnwrapType = UnwrapTypeImpl<std::remove_cvref_t<T>>::Type;


/**
 * 현재 컴파일러에서 사용 가능한 원본 시그니처 정보를 반환합니다.
 * @return 컴파일러에 종속적인 함수 시그니처 문자열
 */
template <typename T>
consteval StringView GetRawTypeSignature() noexcept
{
#if SE_COMPILER_MSVC
    return __FUNCSIG__;
#elif SE_COMPILER_CLANG || SE_COMPILER_GCC
    return __PRETTY_FUNCTION__;
#else
#error "Unsupported compiler for type name extraction"
#endif
}

/** MSVC 시그니처에서 타입 이름을 추출합니다. */
consteval StringView ExtractType_MSVC(StringView signature) noexcept
{
    constexpr StringView prefix_marker = "GetRawTypeSignature<";
    auto start_param = signature.Find(prefix_marker);
    if (!start_param.HasValue())
    {
        return {};
    }
    const usize search_start = *start_param + prefix_marker.ByteLen();

    constexpr StringView suffix_marker = ">(void) noexcept";
    const auto end_param = signature.FindLast(suffix_marker);
    if (!end_param.HasValue() || *end_param <= search_start)
    {
        return {};
    }

    // <>안 Type 정보만 추출
    const StringView type_part = signature.Substr(search_start, *end_param - search_start).Trim();
    return type_part;
}

/** GCC/Clang 시그니처에서 타입 이름을 추출합니다. */
consteval StringView ExtractType_GCC_Clang(StringView signature) noexcept
{
#if SE_COMPILER_CLANG
    constexpr StringView prefix_marker = "[T = ";
#else
    constexpr StringView prefix_marker = "[with T = ";
#endif
    auto start_param = signature.Find(prefix_marker);
    if (!start_param.HasValue())
    {
        return {};
    }
    const usize search_start = *start_param + prefix_marker.ByteLen();


#if SE_COMPILER_CLANG
    constexpr StringView suffix_marker = "]";
#else
    constexpr StringView suffix_marker = ";";
#endif
    const auto end_param = signature.FindLast(suffix_marker);
    if (!end_param.HasValue() || *end_param <= search_start)
    {
        return {};
    }

    // [with T = ;] 또는 [T = ] 안 Type 정보만 추출
    const StringView type_part = signature.Substr(search_start, *end_param - search_start);
    return type_part;
}


/** 컴파일 타임에 템플릿 타입 T의 시그니처에서 이름을 추출합니다. */
template <typename T>
consteval StringView ExtractTypeName() noexcept
{
    constexpr auto signature = GetRawTypeSignature<T>();

#if SE_COMPILER_MSVC
    return ExtractType_MSVC(signature);
#elif SE_COMPILER_CLANG || SE_COMPILER_GCC
    return ExtractType_GCC_Clang(signature);
#else
#error "Unsupported compiler for type name extraction"
#endif
}
}  // namespace detail

/**
 * 컴파일러 시그니처에서 추출된 그대로의 타입 이름을 반환합니다. (예: "class se::MyClass", "struct Foo")
 * @tparam T 이름을 추출할 대상 타입
 */
template <typename T>
[[nodiscard]] consteval StringView GetRawTypeName() noexcept
{
    constexpr auto ret = detail::ExtractTypeName<T>();

    // IDE 버그 때문에 일단 주석
    // static_assert(!ret.empty(), "Failed to extract type name from type T");

    return ret;
}

/**
 * 타입 키워드(struct, class 등)를 제거한 전체 타입 이름을 반환합니다. (예: "se::MyClass")
 * @tparam T 타입 이름을 추출할 대상 타입
 */
template <typename T>
    requires (!se::traits::FunctionType<T>)
[[nodiscard]] consteval StringView GetFullTypeName() noexcept
{
    using CleanType = detail::UnwrapType<T>;
    constexpr auto signature = detail::ExtractTypeName<CleanType>();

    // 선행 타입 키워드 ("class", "struct", "enum", "union") 제거
    constexpr FixedArray<StringView, 5> leading_keywords = { "class", "struct", "enum", "union", "typename" };
    constexpr auto ret = detail::RemoveKeywords(signature, leading_keywords);

    // IDE 버그 때문에 일단 주석
    // static_assert(!ret.empty(), "Failed to extract type name from type T");

    return ret;
}

/**
 * 타입에서 네임스페이스를 제외한 순수 타입 이름을 반환합니다. (예: "MyClass")
 * @warning 템플릿 타입(예: Array<se::String>)에는 사용할 수 없습니다.
 *          RemoveNamespace가 마지막 "::"을 기준으로 잘라내므로 템플릿 인자 안의 "::"에서 잘립니다.
 *          템플릿 타입의 전체 이름이 필요하면 GetFullTypeName<T>()을 사용하세요.
 * @tparam T 타입 이름을 추출할 대상 타입
 */
template <typename T>
    requires (!se::traits::FunctionType<T>)
[[nodiscard]] consteval StringView GetTypeName() noexcept
{
    // TODO: traits::IsTemplateInstance<T> 만들어야 함
    // static_assert(traits::IsTemplateInstance<T>, "GetTypeName<T>() cannot be used with template types. Use GetFullTypeName<T>() instead.");

    constexpr auto ret = GetFullTypeName<T>();
    return detail::RemoveNamespace(ret);
}
}  // namespace se
