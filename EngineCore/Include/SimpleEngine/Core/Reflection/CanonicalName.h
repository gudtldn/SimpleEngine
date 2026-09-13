#pragma once

#include "SimpleEngine/Core/Container/StringView.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Core/Reflection/CompilerFeatures.h"
#include "SimpleEngine/Traits/TypeTraits.h"

#include <concepts>
#include <string>
#include <type_traits>


namespace se::detail
{
template <typename T>
consteval std::string CanonicalNameOf();

#if SE_HAS_REFLECTION
template <typename T>
consteval std::string ReflectCanonicalEntityNameOf(); // CanonicalName_Reflect.h
#else
template <typename T>
consteval std::string FuncSigEntityNameOf(); // CanonicalName_FuncSig.h
#endif

/**
 * 부호 없는 정수를 10진수 문자열로 변환합니다.
 * @param value 변환할 값
 * @return 10진수 문자열
 */
consteval std::string UIntToString(u64 value)
{
    if (value == 0)
    {
        return "0";
    }

    u8 digits[24]{};
    i32 count = 0;
    while (value > 0)
    {
        digits[count++] = static_cast<u8>('0' + (value % 10));
        value /= 10;
    }

    std::string out;
    for (i32 i = count - 1; i >= 0; --i)
    {
        out += static_cast<char>(digits[i]);
    }
    return out;
}

/**
 * Built-In 타입에 대한 이름을 가져옵니다.
 * @tparam T 이름을 가져올 Built-In 타입
 * @return Built-In 타입에 대한 이름
 */
template <typename T>
consteval StringView BuiltinNameOf()
{
    // 기본 타입
    if constexpr (std::same_as<T, void>)                { return "void";      }
    else if constexpr (std::same_as<T, std::nullptr_t>) { return "nullptr_t"; }
    else if constexpr (std::same_as<T, bool>)           { return "bool";      }

    // 문자열 타입
    else if constexpr (std::same_as<T, char>)           { return "char";      }
    else if constexpr (std::same_as<T, char8_t>)        { return "char8";     }
    else if constexpr (std::same_as<T, char16_t>)       { return "char16";    }
    else if constexpr (std::same_as<T, char32_t>)       { return "char32";    }
    else if constexpr (std::same_as<T, wchar_t>)        { return "wchar";     } // 가변 폭이기 때문에 직렬화 금지

    // 실수형 타입
    else if constexpr (std::same_as<T, float>)          { return "f32";       }
    else if constexpr (std::same_as<T, double>)         { return "f64";       }
    else if constexpr (std::same_as<T, long double>)    { return "fext";      } // 가변 폭이기 때문에 직렬화 금지

    // 숫자 타입
    else if constexpr (std::is_integral_v<T>)
    {
        if constexpr (std::is_signed_v<T>)
        {
            if constexpr (sizeof(T) == 1)      { return "i8";  }
            else if constexpr (sizeof(T) == 2) { return "i16"; }
            else if constexpr (sizeof(T) == 4) { return "i32"; }
            else if constexpr (sizeof(T) == 8) { return "i64"; }
            else { static_assert(traits::AlwaysFalse<T>, "CanonicalName: Unsupported signed integer bit-width"); }
        }
        else
        {
            if constexpr (sizeof(T) == 1)      { return "u8";  }
            else if constexpr (sizeof(T) == 2) { return "u16"; }
            else if constexpr (sizeof(T) == 4) { return "u32"; }
            else if constexpr (sizeof(T) == 8) { return "u64"; }
            else { static_assert(traits::AlwaysFalse<T>, "CanonicalName: Unsupported unsigned integer bit-width"); }
        }
    }
    else { static_assert(traits::AlwaysFalse<T>, "CanonicalName: Unhandled primitive type"); }
    return {};
}

/**
 * 함수 타입의 인자 목록을 조립합니다.
 * @tparam Args 인자 타입들
 * @return 인자 목록 문자열
 */
template <typename... Args>
consteval std::string CanonicalArgumentList()
{
    std::string out;
    bool is_first = true;

    [[maybe_unused]] const auto append_argument = [&out, &is_first](std::string&& name)
    {
        if (!is_first)
        {
            out += ',';
        }
        is_first = false;
        out += name;
    };

    (append_argument(CanonicalNameOf<Args>()), ...);

    return out;
}

/**
 * 함수 타입의 이름을 조립합니다.
 * @tparam T 함수 타입
 */
template <typename T>
struct FunctionNameHelper
{
    static_assert(traits::AlwaysFalse<T>, "CanonicalName: Unsupported function type.");
};

template <typename Ret, typename... Args>
struct FunctionNameHelper<Ret(Args...)>
{
    static consteval std::string Get()
    {
        return CanonicalNameOf<Ret>() + "(" + CanonicalArgumentList<Args...>() + ")";
    }
};

template <typename Ret, typename... Args>
struct FunctionNameHelper<Ret(Args...) noexcept>
{
    static consteval std::string Get()
    {
        return CanonicalNameOf<Ret>() + "(" + CanonicalArgumentList<Args...>() + ") noexcept";
    }
};

/**
 * 타입 이름을 수동으로 오버라이드 합니다.
 *
 * 리플렉션이 없는 컴파일러를 지원하는 용도이자, 타입 이름을 리팩터링한 뒤에도
 * 기존 에셋을 살리기 위해 옛 canonical name을 고정하는 마이그레이션 수단입니다.
 *
 * @tparam T 이름을 지정할 타입
 */
template <typename T>
struct TypeNameOverride
{
    static constexpr bool HAS_VALUE = false;
    static constexpr StringView VALUE = {};
};

/**
 * 클래스 / 공용체 / 열거형의 정규화된 이름을 가져옵니다.
 * @tparam T 이름을 가져올 타입
 * @return 스코프를 포함한 정규화된 이름
 */
template <typename T>
consteval std::string CanonicalEntityNameOf()
{
    if constexpr (TypeNameOverride<T>::HAS_VALUE)
    {
        return std::string(TypeNameOverride<T>::VALUE);
    }
    else
    {
#if SE_HAS_REFLECTION
        return ReflectCanonicalEntityNameOf<T>(); // CanonicalName_Reflect.h
#else
        return FuncSigEntityNameOf<T>(); // CanonicalName_FuncSig.h
#endif
    }
}

/**
 * 임의의 타입 T의 정규화된 이름을 조립합니다.
 * @tparam T 이름을 가져올 타입
 * @return 정규화된 이름
 */
template <typename T>
consteval std::string CanonicalNameOf()
{
    // cv 한정자
    if constexpr (std::is_const_v<T> && std::is_volatile_v<T>)
    {
        return CanonicalNameOf<std::remove_cv_t<T>>() + " const volatile";
    }
    else if constexpr (std::is_const_v<T>)
    {
        return CanonicalNameOf<std::remove_const_t<T>>() + " const";
    }
    else if constexpr (std::is_volatile_v<T>)
    {
        return CanonicalNameOf<std::remove_volatile_t<T>>() + " volatile";
    }

    // 참조
    else if constexpr (std::is_lvalue_reference_v<T>)
    {
        return CanonicalNameOf<std::remove_reference_t<T>>() + "&";
    }
    else if constexpr (std::is_rvalue_reference_v<T>)
    {
        return CanonicalNameOf<std::remove_reference_t<T>>() + "&&";
    }

    // 포인터
    else if constexpr (std::is_member_pointer_v<T>)
    {
        // 추후 필요하면 구현
        static_assert(traits::AlwaysFalse<T>, "CanonicalName: Member pointers are not supported yet");
        return {};
    }
    else if constexpr (std::is_pointer_v<T>)
    {
        return CanonicalNameOf<std::remove_pointer_t<T>>() + "*";
    }

    // 배열
    else if constexpr (std::is_bounded_array_v<T>)
    {
        return CanonicalNameOf<std::remove_extent_t<T>>() + "[" + UIntToString(std::extent_v<T>) + "]";
    }
    else if constexpr (std::is_unbounded_array_v<T>)
    {
        return CanonicalNameOf<std::remove_extent_t<T>>() + "[]";
    }

    // 함수
    else if constexpr (std::is_function_v<T>)
    {
        return FunctionNameHelper<T>::Get();
    }

    // Built-In
    else if constexpr (std::is_fundamental_v<T>)
    {
        return std::string(BuiltinNameOf<T>());
    }

    // 클래스 / 공용체 / 열거형
    else
    {
        static_assert(std::is_class_v<T> || std::is_union_v<T> || std::is_enum_v<T>, "CanonicalName: Unknown type category");
        return CanonicalEntityNameOf<T>();
    }
}
} // namespace se::detail

/**
 * 타입의 canonical name을 수동으로 지정합니다.
 * 리플렉션이 없는 컴파일러를 지원하거나, 타입 이름 변경 후에도 기존 에셋을 유지할 때 사용합니다.
 */
#define SE_TYPE_NAME(type, name) \
    template <> \
    struct se::detail::TypeNameOverride<type> \
    { \
        static constexpr bool HAS_VALUE = true; \
        static constexpr ::se::StringView VALUE = name; \
    }
