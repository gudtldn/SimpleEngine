export module SE.Utility:TypeUtils;

import SE.Traits;
import SE.Types;
import std;


namespace se::utility::type
{
namespace string_utils
{
/** 문자열 앞뒤의 공백을 제거합니다. */
consteval std::string_view TrimWhitespace(std::string_view sv) noexcept
{
    constexpr std::string_view whitespace_chars = " \t\n\r\f\v";
    if (sv.empty())
    {
        return sv;
    }
    const size_t first = sv.find_first_not_of(whitespace_chars);
    if (first == std::string_view::npos)
    {
        return {};
    }
    const size_t last = sv.find_last_not_of(whitespace_chars);
    return sv.substr(first, last - first + 1);
}
}

namespace traits
{
template <typename T>
struct RemoveAllQualifiersImpl
{
    using Type = std::remove_pointer_t<std::remove_cv_t<std::remove_all_extents_t<std::remove_reference_t<T>>>>;
};

template <typename T>
struct RemoveAllQualifiersImpl<T*>
{
    using Type = RemoveAllQualifiersImpl<std::remove_cvref_t<T>>::Type;
};

template <typename T>
struct RemoveAllQualifiersImpl<T* const>
{
    using Type = RemoveAllQualifiersImpl<std::remove_cvref_t<T>>::Type;
};

template <typename T>
struct RemoveAllQualifiersImpl<T* volatile>
{
    using Type = RemoveAllQualifiersImpl<std::remove_cvref_t<T>>::Type;
};

template <typename T>
struct RemoveAllQualifiersImpl<T* const volatile>
{
    using Type = RemoveAllQualifiersImpl<std::remove_cvref_t<T>>::Type;
};

/** 재귀적으로 typename T의 한정자를 제거합니다. */
template <typename T>
using RemoveAllQualifiers = RemoveAllQualifiersImpl<T>::Type;
}

namespace detail
{
/** 문자열을 제거할 시작 위치를 지정합니다. */
enum class EQualifierRemovePosition : uint8
{
    FromStart,
    FromEnd,
};

/** 들어온 값이 토큰 경계 문자인지 여부를 구합니다. */
consteval bool IsTokenBoundary(char c)
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
 * @tparam Mode 한정자를 제거할 시작 위치 (FromStart 또는 FromEnd)
 * @tparam N 한정자 배열의 크기
 * @param signature 원본 문자열
 * @param qualifiers 제거할 한정자의 배열
 * @return 한정자가 제거된 시그니처를 반환
 */
template <EQualifierRemovePosition Mode, size_t N>
consteval std::string_view RemoveQualifiers(std::string_view signature, std::array<std::string_view, N> qualifiers) noexcept
{
    bool was_modified;
    do
    {
        was_modified = false;

        for (const std::string_view& qualifier : qualifiers)
        {
            if constexpr (Mode == EQualifierRemovePosition::FromStart)
            {
                if (signature.starts_with(qualifier))
                {
                    // qualifier 다음 문자가 토큰 경계인지 확인
                    const char next = signature.size() > qualifier.size() ? signature[qualifier.size()] : '\0';
                    if (
                        IsTokenBoundary(next)
                        || (qualifier.size() == 1 && IsTokenBoundary(qualifier.front()))
                    )
                    {
                        signature.remove_prefix(qualifier.size());
                        was_modified = true;
                        break;
                    }
                }
            }
            else if constexpr (Mode == EQualifierRemovePosition::FromEnd)
            {
                if (signature.ends_with(qualifier))
                {
                    // qualifier 앞 문자가 토큰 경계인지 확인
                    const char prev = signature.size() > qualifier.size() ? signature[signature.size() - qualifier.size() - 1] : '\0';
                    if (
                        IsTokenBoundary(prev)
                        || (qualifier.size() == 1 && IsTokenBoundary(qualifier.front()))
                    )
                    {
                        signature.remove_suffix(qualifier.size());
                        was_modified = true;
                        break;
                    }
                }
            }
        }

        if (was_modified)
        {
            signature = string_utils::TrimWhitespace(signature);
        }
    } while (was_modified);
    return signature;
}

/** signature에서 namespace를 제거한 원본 타입명을 반환합니다. */
consteval std::string_view RemoveNamespace(std::string_view in_signature) noexcept
{
    constexpr std::string_view namespace_prefix = "::";
    const size_t start_pos = in_signature.rfind(namespace_prefix);
    if (start_pos == std::string_view::npos)
    {
        return in_signature;
    }
    return in_signature.substr(start_pos + namespace_prefix.size());
}

/**
 * 현재 컴파일러에서 사용 가능한 타입 시그니처 정보를 raw 형태로 반환합니다.
 * @return 컴파일러에 종속적인 타입 시그니처 문자열을 반환합니다.
 */
template <typename T>
consteval std::string_view GetRawTypeSignature() noexcept
{
#ifdef _MSC_VER
    return __FUNCSIG__; // NOLINT(clang-diagnostic-language-extension-token)
#elif defined(__clang__) || defined(__GNUC__)
    return __PRETTY_FUNCTION__; // NOLINT(clang-diagnostic-language-extension-token)
#else
#error "Unsupported compiler for type name extraction"
#endif
}

/** MSVC 타입 시그니처에서 원본 타입명을 추출합니다. */
consteval std::string_view ExtractType_MSVC(std::string_view in_signature) noexcept
{
    constexpr std::string_view prefix = "GetRawTypeSignature<";
    size_t start_pos = in_signature.find(prefix);
    if (start_pos == std::string_view::npos)
    {
        return {};
    }
    start_pos += prefix.size();

    constexpr std::string_view suffix = ">(void) noexcept";
    const size_t end_pos = in_signature.rfind(suffix);
    if (end_pos == std::string_view::npos || end_pos <= start_pos)
    {
        return {};
    }

    // <>안 Type 정보만 추출
    std::string_view extracted_typename = string_utils::TrimWhitespace(in_signature.substr(start_pos, end_pos - start_pos));

    // 후행 한정자 (포인터, 참조, cv-한정자) 제거
    constexpr std::array<std::string_view, 4> trailing_qualifiers = { "*", "&", "const", "volatile" };
    extracted_typename = RemoveQualifiers<EQualifierRemovePosition::FromEnd>(extracted_typename, trailing_qualifiers);

    // 선행 cv-한정자 제거
    constexpr std::array<std::string_view, 2> leading_qualifiers = { "const", "volatile" };
    extracted_typename = RemoveQualifiers<EQualifierRemovePosition::FromStart>(extracted_typename, leading_qualifiers);

    // 선행 타입 키워드 ("class", "struct", "enum", "union") 제거
    constexpr std::array<std::string_view, 5> leading_keywords = { "class", "struct", "enum", "union", "typename" };
    extracted_typename = RemoveQualifiers<EQualifierRemovePosition::FromStart>(extracted_typename, leading_keywords);

    return extracted_typename;
}

/** GCC/Clang 타입 시그니처에서 원본 타입명을 추출합니다. */
consteval std::string_view ExtractType_GCC_Clang(std::string_view in_signature) noexcept
{
#if defined(__clang__)
    constexpr std::string_view prefix = "[T = ";
#else
    constexpr std::string_view prefix = "[with T = ";
#endif
    size_t start_pos = in_signature.find(prefix);
    if (start_pos == std::string_view::npos)
    {
        return {};
    }
    start_pos += prefix.size();


#if defined(__clang__)
    constexpr std::string_view suffix = "]";
#else
    constexpr std::string_view suffix = ";";
#endif
    const size_t end_pos = in_signature.rfind(suffix);
    if (end_pos == std::string_view::npos || end_pos <= start_pos)
    {
        return {};
    }

    // [with T = ;]안 Type 정보만 추출
    std::string_view extracted_typename = in_signature.substr(start_pos, end_pos - start_pos);

    // 선행 cv-한정자 제거
    constexpr std::array<std::string_view, 2> leading_qualifiers = { "const", "volatile" };
    extracted_typename = RemoveQualifiers<EQualifierRemovePosition::FromStart>(extracted_typename, leading_qualifiers);

    return extracted_typename;
}


template <typename T>
/** 컴파일 타임에 템플릿 타입 T에서 원본 타입명을 추출합니다. */
consteval std::string_view ExtractTypeName() noexcept
{
    constexpr auto signature = GetRawTypeSignature<T>();

#ifdef _MSC_VER
    return ExtractType_MSVC(signature);
#elif defined(__clang__) || defined(__GNUC__)
    return ExtractType_GCC_Clang(signature);
#else
#error "Unsupported compiler for type name extraction"
#endif
}

template <typename Signature>
struct UnpackTupleImpl;

/**
 * TupleLike에 포함된 모든 타입을 템플릿 파라미터 팩 `Ts...`로 추출(unpack)합니다.
 * @tparam Ts TupleLike으로부터 추출된 타입 파라미터 팩
 */
template <template <typename...> typename TupleLike, typename... Ts>
struct UnpackTupleImpl<TupleLike<Ts...>>
{
    template <typename Fn>
    static constexpr auto Unpack(Fn&& func)
    {
        return std::forward<Fn>(func).template operator()<Ts...>();
    }
};

/**
 * 함수 Signature에 포함된 모든 타입을 템플릿 파라미터 팩 `Ts...`로 추출(unpack)합니다.
 * @tparam Ts 함수 Signature로부터 추출된 타입 파라미터 팩
 */
template <typename R, typename... Ts>
struct UnpackTupleImpl<R(Ts...)>
{
    template <typename Fn>
    static constexpr auto Unpack(Fn&& func)
    {
        return std::forward<Fn>(func).template operator()<Ts...>();
    }
};

/**
 * 두 개의 튜플 타입 T1과 T2를 하나의 튜플 타입으로 병합합니다.
 */
template <typename T1, typename T2>
struct MergeTwoTuples;

template <
    template<typename...> typename TupleLike,
    typename... Ts1,
    typename... Ts2
>
struct MergeTwoTuples<TupleLike<Ts1...>, TupleLike<Ts2...>>
{
    // T1의 타입 목록(Ts1...)과 T2의 타입 목록(Ts2...)을 합쳐 새로운 튜플 타입을 정의
    using Type = TupleLike<Ts1..., Ts2...>;
};

/**
 * 여러 튜플 타입을 재귀적으로 병합하기 위한 구현체
 */
template <typename...>
struct TupleCatImpl;

// 기본 케이스: 튜플에 멤버가 하나도 없으면 빈 튜플을 반환
template <>
struct TupleCatImpl<>
{
    using Type = std::tuple<>;
};

// 기본 케이스: 튜플이 하나만 남으면 그 자신을 결과 타입으로 가짐
template <typename T1>
struct TupleCatImpl<T1>
{
    using Type = T1;
};

// 재귀 케이스: 튜플이 두 개 이상일 때
template <typename T1, typename T2, typename... Rest>
struct TupleCatImpl<T1, T2, Rest...>
{
private:
    // 먼저 앞의 두 튜플을 병합
    using MergedFirstTwo = MergeTwoTuples<T1, T2>::Type;

public:
    // 병합된 결과와 나머지 튜플(Rest...)로 다시 재귀 호출
    using Type = TupleCatImpl<MergedFirstTwo, Rest...>::Type;
};

/**
 * TupleLike 타입을 평탄화하기 위한 구현체
 * @tparam ResultTupleLike 최종 결과물로 사용할 튜플 컨테이너 타입 (예: std::tuple)
 * @tparam T 평탄화할 대상 타입
 */
template <template <typename...> typename ResultTupleLike, typename T>
struct FlattenTupleImpl
{
    // 기본 케이스: T가 튜플이 아닐 때, ResultTupleLike<T>를 반환.
    using Type = ResultTupleLike<T>;
};

template <
    template <typename...> typename ResultTupleLike,
    template <typename...> typename InputTupleLike,
    typename... Ts
>
struct FlattenTupleImpl<ResultTupleLike, InputTupleLike<Ts...>>
{
    // 재귀 케이스: T가 튜플일 때,
    // 각 멤버에 대해 재귀 호출(결과 타입은 ResultTupleLike로 고정)하고, 그 결과들을 TupleCat으로 합침
    using Type = TupleCatImpl<typename FlattenTupleImpl<ResultTupleLike, Ts>::Type...>::Type;
};
}

/**
 * 템플릿 타입 T의 타입 시그니처를 컴파일 타임에 추출합니다.
 * 특정 타입에 대해 네임스페이스 포함 여부를 제어할 수 있습니다.
 *
 * @tparam T 타입 시그니처를 추출할 대상 타입
 * @param include_namespace 네임스페이스를 포함할지 여부를 결정하는 플래그 (기본값: true)
 * @return 추출된 타입 시그니처를 문자열 뷰 형태로 반환합니다.
 */
export template <typename T>
    requires (!se::traits::type_traits::IsFunctionType<T>)
consteval std::string_view GetTypeSignature(bool include_namespace = true) noexcept
{
    using CleanType = traits::RemoveAllQualifiers<T>;
    constexpr std::string_view ret = detail::ExtractTypeName<CleanType>();
    if constexpr (ret.empty())
    {
        static_assert(se::traits::type_traits::AlwaysFalse<T>, "Failed to extract type name from type T");
    }

    if (include_namespace)
    {
        return ret;
    }
    return detail::RemoveNamespace(ret);
}

/**
 * 튜플 타입(`TupleLike`)에서 타입 목록을 추출하여 제네릭 호출 가능 객체(`func`)에 템플릿 인자로 전달합니다.
 * @note std::apply와는 다르게, 인자를 받지 않습니다.
 *
 * @tparam TupleLike 타입들을 추출할 TupleLike (ex: std::tuple<...>)
 * @tparam Fn 템플릿 `operator()`를 가진 제네릭 Lambda 또는 Functor 타입
 * @param func 추출된 타입들을 템플릿 인자로 받아 호출될 객체
 * @return `func`를 호출한 결과값을 그대로 반환
 *
 * @code
 * using MyTuple = std::tuple<int, std::string, double>;
 * auto result = UnpackTuple<MyTuple>([]<typename... TArgs>()
 * {
 *     // 이 블록은 TArgs = int, std::string, double 로 호출됩니다.
 *     return sizeof...(TArgs); // 3을 반환
 * });
 * @endcode
 */
export template <typename TupleLike, typename Fn>
constexpr auto UnpackTuple(Fn&& func)
    requires requires { detail::UnpackTupleImpl<TupleLike>::Unpack(std::forward<Fn>(func)); }
{
    return detail::UnpackTupleImpl<TupleLike>::Unpack(std::forward<Fn>(func));
}

/**
 * 여러 튜플 타입의 멤버 타입들을 모두 포함하는 단일 튜플 타입을 만듭니다.
 * @tparam Tuples 병합할 튜플 타입들
 */
export template <typename... Tuples>
    requires (se::traits::type_traits::IsSpecializationOf<Tuples, std::tuple> && ...)
using TupleCat = detail::TupleCatImpl<Tuples...>::Type;

/**
 * 중첩된 TupleLike 타입을 지정된 컨테이너(기본값: std::tuple)로 평탄화합니다.
 * @tparam T 평탄화할 타입
 * @tparam ResultTupleLike (선택) 결과물로 사용할 튜플 컨테이너.
 *         예: FlattenTuple<MyTuple<...>, MyTuple>
 */
export template <typename T, template <typename...> typename ResultTupleLike = std::tuple>
using FlattenTuple = detail::FlattenTupleImpl<ResultTupleLike, T>::Type;
}
