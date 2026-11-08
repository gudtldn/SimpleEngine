#include "gtest/gtest.h"

#include "SimpleEngine/Core/Container/Optional.h"
#include "SimpleEngine/ECS/QueryConcepts.h"
#include "SimpleEngine/Traits/FunctionTraits.h"
#include "SimpleEngine/Traits/TypeList.h"

#include <tuple>
#include <type_traits>

using namespace se;

namespace
{
struct A {};
struct B {};
struct C {};
struct D {};

using traits::TypeList;

// Size
static_assert(traits::Size<TypeList<>> == 0);
static_assert(traits::Size<TypeList<A, B, C>> == 3);

// Contains, IndexOf는 cv와 참조를 제거하고 비교합니다.
static_assert(traits::Contains<TypeList<const A&>, A>);
static_assert(traits::Contains<TypeList<A>, const A&>);
static_assert(!traits::Contains<TypeList<A>, B>);
static_assert(!traits::Contains<TypeList<>, A>);
static_assert(traits::IndexOf<TypeList<A, B, C>, C> == 2);
static_assert(traits::IndexOf<TypeList<A, const B&, C>, B> == 1);

// IsUnique
static_assert(traits::IsUnique<TypeList<A, B, C>>);
static_assert(traits::IsUnique<TypeList<>>);
static_assert(!traits::IsUnique<TypeList<A, const A&>>);
static_assert(!traits::IsUnique<TypeList<A, B, A>>);
static_assert(traits::IsUnique<std::tuple<A, B>>);
static_assert(!traits::IsUnique<const std::tuple<A, A&>>);

// IsDisjoint
static_assert(traits::IsDisjoint<TypeList<A, B>, TypeList<C, D>>);
static_assert(!traits::IsDisjoint<TypeList<A, B>, TypeList<const B&, D>>);
static_assert(traits::IsDisjoint<std::tuple<A>, std::tuple<B>>);

// Concat
static_assert(std::same_as<traits::Concat<>, TypeList<>>);
static_assert(std::same_as<traits::Concat<TypeList<A>>, TypeList<A>>);
static_assert(std::same_as<traits::Concat<TypeList<A>, TypeList<>, TypeList<B, C>>, TypeList<A, B, C>>);

// Flatten은 TypeList만 펼칩니다.
static_assert(std::same_as<
    traits::Flatten<TypeList<A, TypeList<B, C>, Optional<D>>>,
    TypeList<A, B, C, Optional<D>>
>);
static_assert(std::same_as<traits::Flatten<TypeList<TypeList<A, TypeList<B>>, TypeList<>>>, TypeList<A, B>>);
static_assert(std::same_as<traits::Flatten<TypeList<std::tuple<A, B>>>, TypeList<std::tuple<A, B>>>);

// Map
static_assert(std::same_as<traits::Map<TypeList<A, B>, std::add_const_t>, TypeList<const A, const B>>);

// Rebind
static_assert(std::same_as<traits::Rebind<TypeList<A, B>, std::tuple>, std::tuple<A, B>>);

// FunctionTraits::ArgumentTypes
static_assert(std::same_as<traits::FunctionTraits<void(A, const B&)>::ArgumentTypes, TypeList<A, const B&>>);

// Query 컴포넌트 목록
static_assert(std::same_as<detail::QueryComponents<A&, const B&>, TypeList<A, B>>);
static_assert(std::same_as<detail::QueryComponents<A&, Optional<const B&>, With<C>, Without<D>>, TypeList<A, B, C, D>>);
static_assert(traits::IsUnique<detail::QueryComponents<A&, const B&, With<C>, Without<D>>>);

// Query 인자 사이의 중복은 그대로 잡혀야 합니다.
static_assert(!traits::IsUnique<detail::QueryComponents<A&, Optional<const A&>>>);
static_assert(!traits::IsUnique<detail::QueryComponents<A, With<A>>>);
static_assert(!traits::IsUnique<detail::QueryComponents<A, Without<A>>>);
} // namespace

TEST(TypeListTest, ApplyTypesPassesTypesToLambda)
{
    // Arrange
    using List = TypeList<A, B, C>;

    // Act
    const usize count = traits::ApplyTypes<List>([]<typename... Ts>
    {
        return sizeof...(Ts);
    });

    // Assert
    EXPECT_EQ(count, 3u);
}
