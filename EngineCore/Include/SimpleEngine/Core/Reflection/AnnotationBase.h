#pragma once

#include "SimpleEngine/Core/Container/FixedArray.h"
#include "SimpleEngine/Core/Reflection/TypeId.h"
#include "SimpleEngine/Core/Reflection/TypeShape.h"

#include <tuple>
#include <utility>


namespace se::detail
{
/** static 수명의 태그 값 튜플(&values)을 받아, 각 원소의 주소를 담은 AnnotationRef 배열로 바꿉니다. */
template <typename Tuple>
consteval auto MakeRefs(const Tuple* values)
{
    return [values]<usize... I>(std::index_sequence<I...>)
    {
        return FixedArray<AnnotationRef, sizeof...(I)>{
            AnnotationRef{
                .tag = TypeId::Of<std::tuple_element_t<I, Tuple>>(),
                .value = &std::get<I>(*values),
            }...
        };
    }(std::make_index_sequence<std::tuple_size_v<Tuple>>{});
}
} // namespace se::detail
