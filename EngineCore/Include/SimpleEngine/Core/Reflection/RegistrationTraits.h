#pragma once

#include "SimpleEngine/Traits/TypeTraits.h"

#include <tuple>


namespace se
{
/**
 * 어노테이션이 붙은 타입이 등록된 뒤 실행할 훅
 * 특수화 시 OnTypeRegistered<T>(const Annotation&)를 정의해야 합니다.
 */
template <typename Annotation>
struct RegistrationTraits
{
    using UnspecializedMarker = void;
};

namespace detail
{
/** 타입 어노테이션 중 RegistrationTraits가 특수화된 것마다 OnTypeRegistered<T>(annotation)를 부릅니다. */
template <typename T, typename... Annotations>
void RunRegistrationTraits(const std::tuple<Annotations...>& annotations)
{
    auto process = []<typename Annotation>(const Annotation& annotation)
    {
        if constexpr (traits::IsSpecialized<RegistrationTraits, Annotation>)
        {
            static_assert(
                requires { RegistrationTraits<Annotation>::template OnTypeRegistered<T>(annotation); },
                "RegistrationTraits<Annotation> is specialized, but it does not provide "
                "a valid static 'template <typename T> void OnTypeRegistered(const Annotation&)' function."
            );
            RegistrationTraits<Annotation>::template OnTypeRegistered<T>(annotation);
        }
    };
    (process(std::get<Annotations>(annotations)), ...);
}
} // namespace detail
} // namespace se
