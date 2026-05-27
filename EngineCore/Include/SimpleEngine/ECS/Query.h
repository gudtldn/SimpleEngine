#pragma once

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/Optional.h"
#include "SimpleEngine/ECS/QueryConcepts.h"
#include "SimpleEngine/ECS/QueryData.h"
#include "SimpleEngine/Traits/ContainerTraits.h"
#include "SimpleEngine/Traits/TupleTraits.h"
#include "SimpleEngine/Utility/Debug.h"

#include <concepts>
#include <type_traits>
#include <utility>


namespace se
{
namespace detail
{
/** 쿼리 파라미터(T)의 읽기/쓰기 권한에 맞는 Pool(SparseSet) 포인터 타입을 추론합니다. */
template <typename T>
using PoolPtrType = std::conditional_t<
    std::same_as<std::remove_cvref_t<T>, Entity>,
    EmptyType, // Entity는 Pool이 필요 없음
    std::conditional_t<
        IsReadOnlyType<T>::Value,
        const SparseSet<std::remove_cvref_t<traits::InnerOf<T>>>*, // 읽기 전용 쿼리 -> const Pool
        SparseSet<std::remove_cvref_t<traits::InnerOf<T>>>*        // 쓰기 가능 쿼리 -> 일반 Pool
    >
>;

/** World에서 캐싱 용도로 사용할 SparseSet 포인터를 추출합니다. */
template <typename TargetWorld, typename T>
static PoolPtrType<T> GetPoolPtr(TargetWorld& world)
{
    // Entity는 무시
    if constexpr (std::same_as<std::remove_cvref_t<T>, Entity>)
    {
        return EmptyType{};
    }
    else
    {
        using ComponentType = std::remove_cvref_t<traits::InnerOf<T>>;

        auto opt = world.template FindSparseSet<ComponentType>();
        return opt ? std::addressof(*opt) : nullptr;
    }
}
} // namespace detail

/**
 * 조건에 맞는 Entity와 컴포넌트들을 순회(iterate)하기 위한 인터페이스입니다.
 * @tparam Ts 조회할 컴포넌트 타입들
 */
template <typename... Ts>
class Query
{
    using Validator = detail::QueryValidator<Ts...>;

    static_assert(Validator::HasElements,
        "Query requires at least one component or filter tag.");

    static_assert(Validator::IsUnique,
        "Query parameters must be unique. Duplicate types are not allowed.");

    static_assert(Validator::NoPointers,
        "Raw pointers are not permitted in Query. Use value (T) or reference (T&) types.");

    static_assert(Validator::AllValidComponents,
        "Query parameters must be valid component types (class/struct) or filter tags (With/Without).");

    static_assert(Validator::NoEntityReference,
        "Entity in a Query must be fetched by value, not by reference.");

    static_assert(Validator::NoOptionalReference,
        "Optional<T> in a Query must not have an outer reference. "
        "Use Optional<T&> for mutable access, not Optional<T>&.");

    static_assert(Validator::NoOptionalEntity,
        "Optional<Entity> is not allowed in a Query. "
        "Entity is not stored as a component and has no pool.");

private:
    friend class Iterator;

    /** 필터링 조건 및 스토리지 캐싱 관리 */
    using QueryDataType = detail::QueryData<Ts...>;

    /** Query 권한에 따른 월드 타입 (const World / World) */
    using TargetWorld = QueryDataType::TargetWorld;

    /** 순회 시 반환할 컴포넌트 팩 (Tuple) */
    using FetchTypes = QueryDataType::FetchTypes;

    /** 조회를 위한 SparseSet 포인터 Tuple */
    template <typename T>
    using PoolPtrWrapper = detail::PoolPtrType<T>;
    using FetchPoolsTuple = traits::TupleMap<FetchTypes, PoolPtrWrapper>;

    /** 쿼리의 순회 범위가 특정 컴포넌트로 제한되는지 여부 (필수 컴포넌트 존재 여부) */
    static constexpr bool IS_COMPONENT_RESTRICTED = QueryDataType::NUM_PREDICATES > 0;

    /** 순회 대상 소스 타입: 제한된 경우 IComponentStorage(Pool), 아닌 경우 전체 Entity 배열 */
    using IterationSourceType = std::conditional_t<IS_COMPONENT_RESTRICTED, const IComponentStorage*, const Array<Entity>*>;

public:
    explicit Query(TargetWorld& in_world)
        : query_data(in_world)
    {
        if constexpr (IS_COMPONENT_RESTRICTED)
        {
            iteration_source = query_data.FindSmallestPool();
        }
        else
        {
            iteration_source = &in_world.GetAliveEntities();
        }

        // FetchPool 캐싱
        fetch_pools = traits::ApplyTypes<FetchTypes>([&in_world]<typename... FetchTs>
        {
            return FetchPoolsTuple{ detail::GetPoolPtr<TargetWorld, FetchTs>(in_world)... };
        });
    }

    ~Query() = default;

    Query(const Query&) = delete;
    Query& operator=(const Query&) = delete;
    Query(Query&&) noexcept = default;
    Query& operator=(Query&&) noexcept = default;

public:
    /** 특정 Entity가 쿼리 조건을 만족하는 경우, 해당 컴포넌트들을 반환합니다. */
    [[nodiscard]] Optional<FetchTypes> TryGet(Entity entity) const
    {
        if (query_data.IsEntityValid(entity))
        {
            return [this, entity]<usize... Is>(std::index_sequence<Is...>)
            {
                return FetchTypes{ FetchComponent<std::tuple_element_t<Is, FetchTypes>, Is>(entity)... };
            }(std::make_index_sequence<std::tuple_size_v<FetchTypes>>{});
        }
        return NullOpt;
    }

    /** 쿼리 결과가 정확히 하나일 때만 컴포넌트들을 Optional로 반환합니다. 결과가 없거나 여러 개이면 nullopt를 반환합니다. */
    [[nodiscard]] Optional<FetchTypes> TryGetSingle() const
    {
        auto it = begin();
        if (it == end())
        {
            return NullOpt;
        }

        FetchTypes result = *it;
        ++it;
        if (it != end())
        {
            return NullOpt;
        }

        return result;
    }

    /** 쿼리 결과가 정확히 하나일 때만 컴포넌트들을 반환합니다. 결과가 없거나 여러 개이면 assert로 프로그램을 중단시킵니다. */
    [[nodiscard]] FetchTypes GetSingle() const
    {
        auto it = begin();
        SE_ASSERT_RELEASE(it != end(), "Called GetSingle() on a query with no matching entities.");

        FetchTypes result = *it;
        ++it;
        SE_ASSERT_RELEASE(it == end(), "Called GetSingle() on a query with more than one matching entity.");

        return result;
    }

    /** 쿼리 결과가 비어있는지 확인합니다. */
    [[nodiscard]] bool IsEmpty() const
    {
        return begin() == end();
    }

private:
    /** Query<Ts...>의 Ts... 부분에 맞는 값을 반환하는 헬퍼 함수 */
    template <typename T, usize Idx>
    T FetchComponent(Entity entity) const
    {
        using RawType = std::remove_cvref_t<T>;

        if constexpr (std::same_as<RawType, Entity>)
        {
            return entity;
        }
        else if constexpr (traits::OptionalLike<RawType>)
        {
            auto* pool = std::get<Idx>(fetch_pools);
            if (!pool)
            {
                return NullOpt;
            }

            using InnerType = traits::InnerOf<RawType>;
            if constexpr (std::is_reference_v<InnerType>)
            {
                return pool->Find(entity);
            }
            else
            {
                // Optional<T&>를 Optional<T>로 복사하여 반환
                return pool->Find(entity).Copy();
            }
        }
        else
        {
            auto* pool = std::get<Idx>(fetch_pools);
            SE_ASSERT(pool != nullptr, "Query fetch failed: Required component pool is missing!");
            return pool->Get(entity);
        }
    }

public:
    class Iterator
    {
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type = FetchTypes;
        using difference_type = std::ptrdiff_t;

    public:
        Iterator(const Query* in_query, usize in_index)
            : query(in_query)
            , storage_index(in_index)
            , iteration_source(in_query->iteration_source)
        {
            AdvanceToValid();
        }

        value_type operator*() const noexcept
        {
            Entity entity;
            if constexpr (IS_COMPONENT_RESTRICTED)
            {
                entity = iteration_source->GetEntityByIndex(storage_index).Value();
            }
            else
            {
                entity = (*iteration_source)[storage_index];
            }

            // 해시맵 조회 없이 인덱스 시퀀스를 이용해 다이렉트 접근
            return [this, entity]<usize... Is>(std::index_sequence<Is...>)
            {
                return value_type{ query->FetchComponent<std::tuple_element_t<Is, FetchTypes>, Is>(entity)... };
            }(std::make_index_sequence<std::tuple_size_v<FetchTypes>>{});
        }

        Iterator& operator++()
        {
            ++storage_index;
            AdvanceToValid();
            return *this;
        }

        bool operator==(const Iterator& other) const noexcept
        {
            return iteration_source == other.iteration_source && storage_index == other.storage_index;
        }

    private:
        void AdvanceToValid()
        {
            if constexpr (IS_COMPONENT_RESTRICTED)
            {
                if (!iteration_source) [[unlikely]]
                {
                    // FindSmallestPool이 nullptr을 반환하는 경우 (예: 해당 컴포넌트를 가진 Entity가 없음)
                    storage_index = 0;
                    return;
                }
                while (storage_index < iteration_source->Len())
                {
                    if (const auto entity = iteration_source->GetEntityByIndex(storage_index))
                    {
                        if (query->query_data.IsEntityValid(*entity))
                        {
                            return;
                        }
                    }
                    ++storage_index;
                }
            }
            else
            {
                SE_ASSERT(iteration_source);

                // ReSharper disable once CppDFANullDereference
                const auto& entities = *iteration_source;
                while (storage_index < entities.Len())
                {
                    if (query->query_data.IsEntityValid(entities[storage_index]))
                    {
                        return;
                    }
                    ++storage_index;
                }
            }
        }

    private:
        const Query* query;
        usize storage_index;
        IterationSourceType iteration_source;
    };

    [[nodiscard]] Iterator begin() const
    {
        return Iterator{ this, 0 };
    }

    [[nodiscard]] Iterator end() const
    {
        usize end_index = iteration_source ? iteration_source->Len() : 0;
        return Iterator{ this, end_index };
    }

private:
    QueryDataType query_data;
    FetchPoolsTuple fetch_pools;
    IterationSourceType iteration_source;
};
} // namespace se
