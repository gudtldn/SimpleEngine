module;
#include "tracy/Tracy.hpp"
export module SE.Core:ECS.Query;
import :ECS.World;

import SE.Types;
import SE.Traits;
import SE.Utility;
import std;

#define SE_DEFINE_TYPE_CONDITION_TAG(tag_name, condition) \
template <typename T> \
struct tag_name { static constexpr bool Value = condition; }

using namespace se::traits::type_traits;
using namespace se::utility::type;

namespace se::core::ecs
{
export inline namespace filters
{
/**
 * 쿼리 결과에 반드시 포함되어야 하는 Component를 지정하는 FilterTag
 * @tparam ComponentTypes 필터링할 Component 타입들
 */
template <typename... ComponentTypes>
struct With
{
    using Types = std::tuple<ComponentTypes...>;
};

/**
 * 쿼리 결과에서 반드시 제외되어야 하는 Component를 지정하는 FilterTag
 * @tparam ComponentTypes 필터링할 Component 타입들
 */
template <typename... ComponentTypes>
struct Without
{
    using Types = std::tuple<ComponentTypes...>;
};
}

namespace details
{
template <typename T, template <typename...> typename... Ts>
concept IsSpecializationTypes = (IsSpecializationOf<T, Ts> || ...);

// 타입 T가 FilterTag인지 확인하는 Concept
template <typename T>
concept IsFilterTag = IsSpecializationTypes<T, With, Without>;

template <template <typename> typename ConditionTag, typename... Ts>
    requires requires { (ConditionTag<Ts>::Value, ...); }
using ExtractTypes = TupleCat<
    std::conditional_t<ConditionTag<Ts>::Value, std::tuple<Ts>, std::tuple<>>...
>;

SE_DEFINE_TYPE_CONDITION_TAG(CondFetchTag, !IsFilterTag<T>);
SE_DEFINE_TYPE_CONDITION_TAG(CondFilterTag, IsFilterTag<T>);
SE_DEFINE_TYPE_CONDITION_TAG(CondWithTag, (IsSpecializationOf<T, With>));
SE_DEFINE_TYPE_CONDITION_TAG(CondWithoutTag, (IsSpecializationOf<T, Without>));

template <typename TupleType>
struct TupleHasPointerTypesImpl;

template <
    template <typename...> typename TupleLike,
    typename... Ts
>
struct TupleHasPointerTypesImpl<TupleLike<Ts...>>
{
    static constexpr bool Value = (std::is_pointer_v<Ts> || ...);
};

template <typename TupleType>
concept TupleHasPointerTypes = TupleHasPointerTypesImpl<TupleType>::Value;
}

/**
 * ECS 월드에서 Entity와 Component를 조회하기 위한 Query 인터페이스
 */
export template <typename... Ts>
    requires (sizeof...(Ts) > 0)
    && TupleHasUniqueTypes<FlattenTuple<std::tuple<Ts...>>> // 입력된 타입들이 겹치면 안됨
class Query
{
    friend class Iterator;

    using FetchTypes = details::ExtractTypes<details::CondFetchTag, Ts...>; // std::tuple<Comp...>
    using WithTypes = FlattenTuple<details::ExtractTypes<details::CondWithTag, Ts...>>;
    using WithoutTypes = FlattenTuple<details::ExtractTypes<details::CondWithoutTag, Ts...>>;

    // 쿼리 타입으로 포인터 타입이 들어오면 안됨
    static_assert(
        !details::TupleHasPointerTypes<TupleMap<FlattenTuple<std::tuple<Ts...>>, std::decay_t>>,
        "No pointer types are allowed in the query parameters."
    );

public:
    explicit Query(World* in_world);
    ~Query() = default;

    Query(const Query&) = default;
    Query& operator=(const Query&) = default;
    Query(Query&&) = default;
    Query& operator=(Query&&) = default;

public:

private:
    /** Fetch와 With 목록의 모든 SparseSet중 가장 작은 것을 찾아 반환합니다. */
    IStorage* FindSmallestPool();

public:
    class Iterator
    {
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type = FetchTypes;
        using difference_type = std::ptrdiff_t;

    public:
        Iterator(World* in_world, IStorage* in_pool, size_t in_index)
            : world(in_world)
            , base_pool(in_pool)
            , storage_index(in_index)
        {
            AdvanceToValid();
        }

        value_type operator*() const noexcept
        {
            Entity entity = *base_pool->GetEntityByIndex(storage_index);
            return utility::type::WithUnpackedTypes<FetchTypes>([this, entity]<typename... FetchComps>
            {
                return value_type{
                    world->GetComponent<std::decay_t<FetchComps>>(entity)...
                };
            });
        }

        Iterator& operator++()
        {
            ++storage_index;
            AdvanceToValid();
            return *this;
        }

        bool operator==(const Iterator& other) const noexcept
        {
            return storage_index == other.storage_index && base_pool == other.base_pool;
        }

    private:
        void AdvanceToValid()
        {
            ZoneScoped;

            // 기준 풀이 없거나, 인덱스가 끝에 도달했으면 즉시 종료
            if (!base_pool || storage_index >= base_pool->Length())
            {
                return;
            }

            while (storage_index < base_pool->Length())
            {
                if (Optional<Entity> entity_opt = base_pool->GetEntityByIndex(storage_index))
                {
                    Entity entity = *entity_opt;

                    // Fetch와 With에 있는 Component를 가지고 있는지 확인
                    // TODO: 로직 최적화
                    const bool has_all_required =
                        WithUnpackedTypes<FetchTypes>([this, entity]<typename... FetchComps>
                        {
                            return (world->HasComponent<std::decay_t<FetchComps>>(entity) && ...);
                        })
                        && WithUnpackedTypes<WithTypes>([this, entity]<typename... WithComps>
                        {
                            return (world->HasComponent<std::decay_t<WithComps>>(entity) && ...);
                        });

                    if (!has_all_required)
                    {
                        ++storage_index;
                        continue;
                    }

                    // Without도 가지고 있는지 검사
                    const bool has_any_excluded =
                        WithUnpackedTypes<WithoutTypes>([this, entity]<typename... WithoutComps>
                        {
                            return (world->HasComponent<std::decay_t<WithoutComps>>(entity) || ...);
                        });

                    if (has_any_excluded)
                    {
                        ++storage_index;
                        continue;
                    }

                    // 유효한 엔티티면 return
                    return;
                }
                ++storage_index;
            }
        }

    private:
        World* world;
        IStorage* base_pool;
        size_t storage_index;
    };

    Iterator begin()
    {
        IStorage* smallest_pool = FindSmallestPool();
        return Iterator(world, smallest_pool, 0);
    }

    Iterator end()
    {
        IStorage* smallest_pool = FindSmallestPool();
        const size_t end_index = smallest_pool ? smallest_pool->Length() : 0;
        return Iterator(world, smallest_pool, end_index);
    }

private:
    World* world;
};

template <typename... Ts>
    requires (sizeof...(Ts) > 0)
    && TupleHasUniqueTypes<FlattenTuple<std::tuple<Ts...>>>
Query<Ts...>::Query(World* in_world)
    : world(in_world)
{
}

template <typename... Ts>
    requires (sizeof...(Ts) > 0)
    && TupleHasUniqueTypes<FlattenTuple<std::tuple<Ts...>>>
IStorage* Query<Ts...>::FindSmallestPool()
{
    constexpr size_t pool_size =
        std::tuple_size_v<FetchTypes>
        + std::tuple_size_v<WithTypes>;

    if constexpr (pool_size == 0)
    {
        return nullptr;
    }

    std::array<IStorage*, pool_size> pools;
    size_t i = 0;

    WithUnpackedTypes<FetchTypes>([this, &pools, &i]<typename... FetchComps>
    {
        ((pools[i++] = world->GetIStorage<std::decay_t<FetchComps>>()), ...);
    });
    WithUnpackedTypes<WithTypes>([this, &pools, &i]<typename... WithComps>
    {
        ((pools[i++] = world->GetIStorage<std::decay_t<WithComps>>()), ...);
    });

    return *std::ranges::min_element(pools, [](const IStorage* a, const IStorage* b)
    {
        if (!a)
        {
            return false;
        }
        if (!b)
        {
            return true;
        }
        return a->Length() < b->Length();
    });
}
}
