// C++26 전환 시 이 파일 전체를 삭제하고 CanonicalName_Reflect.h로 교체
#pragma once

#include "SimpleEngine/Core/Container/StringView.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Traits/TypeTraits.h"

#include <string>


namespace se::detail
{
/** 컴파일러가 생성한 원시 시그니처(__FUNCSIG__)에서 타입 T 부분만 잘라냅니다. */
template <typename T>
consteval StringView RawEntitySignatureOf() noexcept
{
    constexpr StringView sig = __FUNCSIG__;

    constexpr StringView prefix_marker = "RawEntitySignatureOf<";
    constexpr auto start = sig.Find(prefix_marker);

    constexpr StringView suffix_marker = ">(void) noexcept";
    constexpr auto end = sig.FindLast(suffix_marker);

    if constexpr (!start.HasValue() || !end.HasValue() || *end <= *start)
    {
        return {};
    }

    constexpr usize begin_pos = *start + prefix_marker.ByteLen();
    return sig.Substr(begin_pos, *end - begin_pos).Trim();
}

/** class / struct / enum / union 선행 키워드를 제거합니다. */
consteval StringView StripLeadingKeyword(StringView name) noexcept
{
    constexpr StringView keywords[] = { "class ", "struct ", "enum ", "union " };
    for (const StringView keyword : keywords)
    {
        if (name.StartsWith(keyword))
        {
            return name.Substr(keyword.ByteLen());
        }
    }
    return name;
}

/** 템플릿 인자가 없는(또는 head-cut 만으로 처리 불가능한) 타입에 대한 기본 구현입니다. */
template <typename T>
struct TemplateArgsOf
{
    static consteval std::string Get() // NOLINT(*-use-string-view)
    {
        static_assert(traits::AlwaysFalse<T>,
            "CanonicalName_FuncSig: template arguments could not be recovered "
            "(non-type template parameter mixed in?). Use SE_TYPE_NAME to override.");
        return {};
    }
};

/** 타입 전용 템플릿(Tmpl<Args...>)의 인자를 CanonicalNameOf 재귀로 조립합니다. */
template <template <typename...> class Tmpl, typename... Args>
struct TemplateArgsOf<Tmpl<Args...>>
{
    static consteval std::string Get()
    {
        return "<" + CanonicalArgumentList<Args...>() + ">";
    }
};

/** FixedArray<T, N>처럼 크기를 비타입 인자로 받는 템플릿의 인자를 조립합니다. */
template <template <typename, usize> class Tmpl, typename T, usize N>
struct TemplateArgsOf<Tmpl<T, N>>
{
    static consteval std::string Get()
    {
        return "<" + CanonicalNameOf<T>() + "," + UIntToString(N) + ">";
    }
};

/** FixedString<N>, HashDigest<N>처럼 크기만 비타입 인자로 받는 템플릿의 인자를 조립합니다. */
template <template <usize> class Tmpl, usize N>
struct TemplateArgsOf<Tmpl<N>>
{
    static consteval std::string Get()
    {
        return "<" + UIntToString(N) + ">";
    }
};

/** head-cut 기법으로 엔티티(클래스/공용체/열거형) 이름을 조립합니다. */
template <typename T>
consteval std::string FuncSigEntityNameOf()
{
    constexpr StringView entity = StripLeadingKeyword(RawEntitySignatureOf<T>());

    static_assert(!entity.IsEmpty(), "CanonicalName_FuncSig: failed to extract entity name from __FUNCSIG__");
    static_assert(!entity.Contains("<lambda"), "CanonicalName_FuncSig: lambda closures have no linkage and cannot be reflected");
    static_assert(!entity.Contains('`'), "CanonicalName_FuncSig: anonymous namespace / local types cannot be reflected");

    constexpr auto first_angle = entity.Find('<');
    if constexpr (!first_angle.HasValue())
    {
        // 템플릿이 아닌 일반 엔티티
        return std::string(entity);
    }
    else
    {
        static_assert(entity.EndsWith('>'),
            "CanonicalName_FuncSig: nested type inside a template specialization is not supported "
            "(e.g. Grid<f32>::Cell). Use SE_TYPE_NAME to override.");

        constexpr StringView head_name = entity.Substr(0, *first_angle);
        return std::string(head_name) + TemplateArgsOf<T>::Get();
    }
}
} // namespace se::detail
