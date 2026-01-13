#pragma once
#include <array>
#include <string_view>

#include "SimpleEngine/Traits/TypeTraits.h"


namespace se::refl
{
namespace details
{
/** 문자열 앞뒤의 공백을 제거합니다. */
[[nodiscard]] consteval std::string_view TrimWhitespace(std::string_view sv) noexcept
{
    constexpr std::string_view whitespace_chars = " \t\n\r\f\v";
    const usize first = sv.find_first_not_of(whitespace_chars);
    if (first == std::string_view::npos)
    {
        return {};
    }
    const usize last = sv.find_last_not_of(whitespace_chars);
    return sv.substr(first, last - first + 1);
}

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
consteval std::string_view RemoveKeywords(std::string_view signature, const std::array<std::string_view, N>& keywords) noexcept
{
    bool modified;
    do
    {
        modified = false;

        for (const std::string_view& keyword : keywords)
        {
            if (signature.starts_with(keyword))
            {
                // qualifier 다음 문자가 토큰 경계인지 확인
                const char next = signature.size() > keyword.size() ? signature[keyword.size()] : '\0';
                if (IsTokenBoundary(next))
                {
                    signature.remove_prefix(keyword.size());
                    modified = true;
                    break;
                }
            }
        }

        if (modified)
        {
            signature = TrimWhitespace(signature);
        }
    }
    while (modified);
    return signature;
}

/** signature에서 namespace를 제거한 원본 타입명을 반환합니다. */
// ReSharper disable once CppDFAUnreachableFunctionCall
consteval std::string_view RemoveNamespace(std::string_view signature) noexcept
{
    constexpr std::string_view ns_sep = "::";
    const usize pos = signature.rfind(ns_sep);
    if (pos == std::string_view::npos)
    {
        return signature;
    }
    return signature.substr(pos + ns_sep.size());
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
consteval std::string_view GetRawTypeSignature() noexcept
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
consteval std::string_view ExtractType_MSVC(std::string_view signature) noexcept
{
    constexpr std::string_view prefix = "GetRawTypeSignature<";
    usize start_pos = signature.find(prefix);
    if (start_pos == std::string_view::npos)
    {
        return {};
    }
    start_pos += prefix.size();

    constexpr std::string_view suffix = ">(void) noexcept";
    const usize end_pos = signature.rfind(suffix);
    if (end_pos == std::string_view::npos || end_pos <= start_pos)
    {
        return {};
    }

    // <>안 Type 정보만 추출
    const std::string_view extracted_typename = TrimWhitespace(signature.substr(start_pos, end_pos - start_pos));
    return extracted_typename;
}

/** GCC/Clang 시그니처에서 타입 이름을 추출합니다. */
consteval std::string_view ExtractType_GCC_Clang(std::string_view signature) noexcept
{
#if SE_COMPILER_CLANG
    constexpr std::string_view prefix = "[T = ";
#else
    constexpr std::string_view prefix = "[with T = ";
#endif
    usize start_pos = signature.find(prefix);
    if (start_pos == std::string_view::npos)
    {
        return {};
    }
    start_pos += prefix.size();


#if SE_COMPILER_CLANG
    constexpr std::string_view suffix = "]";
#else
    constexpr std::string_view suffix = ";";
#endif
    const usize end_pos = signature.rfind(suffix);
    if (end_pos == std::string_view::npos || end_pos <= start_pos)
    {
        return {};
    }

    // [with T = ;]안 Type 정보만 추출
    const std::string_view extracted_typename = signature.substr(start_pos, end_pos - start_pos);
    return extracted_typename;
}


/** 컴파일 타임에 템플릿 타입 T의 시그니처에서 이름을 추출합니다. */
template <typename T>
consteval std::string_view ExtractTypeName() noexcept
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
}  // namespace details

/**
 * 컴파일러 시그니처에서 추출된 그대로의 타입 이름을 반환합니다. (예: "class se::MyClass", "struct Foo")
 * @tparam T 이름을 추출할 대상 타입
 */
template <typename T>
[[nodiscard]] consteval std::string_view GetRawTypeName() noexcept
{
    constexpr auto ret = details::ExtractTypeName<T>();

    // IDE 버그 때문에 일단 주석
    // static_assert(!ret.empty(), "Failed to extract type name from type T");

    return ret;
}

/**
 * 타입 키워드(struct, class 등)를 제거한 전체 타입 이름을 반환합니다. (예: "se::MyClass")
 * @tparam T 타입 이름을 추출할 대상 타입
 */
template <typename T>
    requires (!se::traits::IsFunctionType<T>)
[[nodiscard]] consteval std::string_view GetFullTypeName() noexcept
{
    using CleanType = details::UnwrapType<T>;
    constexpr auto signature = details::ExtractTypeName<CleanType>();

    // 선행 타입 키워드 ("class", "struct", "enum", "union") 제거
    constexpr std::array<std::string_view, 5> leading_keywords = { "class", "struct", "enum", "union", "typename" };
    constexpr auto ret = details::RemoveKeywords(signature, leading_keywords);

    // IDE 버그 때문에 일단 주석
    // static_assert(!ret.empty(), "Failed to extract type name from type T");

    return ret;
}

/**
 * 타입에서 네임스페이스를 제외한 순수 타입 이름을 반환합니다. (예: "MyClass")
 * @tparam T 타입 이름을 추출할 대상 타입
 */
template <typename T>
    requires (!se::traits::IsFunctionType<T>)
[[nodiscard]] consteval std::string_view GetTypeName() noexcept
{
    constexpr auto ret = GetFullTypeName<T>();
    return details::RemoveNamespace(ret);
}
}  // namespace se::refl
