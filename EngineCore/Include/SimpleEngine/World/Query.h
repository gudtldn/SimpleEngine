#pragma once
#include <concepts>
#include <type_traits>
#include <utility>
#include <variant>

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/Optional.h"
#include "SimpleEngine/World/QueryConcepts.h"
#include "SimpleEngine/World/QueryData.h"

#include "tracy/Tracy.hpp"


namespace se::world
{
/**
 * 조건에 맞는 엔티티와 컴포넌트들을 순회(iterate)하기 위한 인터페이스입니다.
 * @tparam Ts 조회할 컴포넌트 타입들
 */
template <typename... Ts>
    requires QueryParameterPack<Ts...>
class Query
{
    friend class Iterator;

    using QueryDataType = QueryData<Ts...>;
    using FetchTypes = QueryDataType::FetchTypes;

    static constexpr bool HasBasePool = std::tuple_size_v<typename QueryDataType::PredicateTypes> > 0;

    struct EmptyCache{};
    using CacheType = std::conditional_t<HasBasePool, EmptyCache, Array<Entity>>;

public:
    explicit Query(World* in_world)
        : query_data(in_world)
    {
        if constexpr (!HasBasePool)
        {
            alive_entities_cache = query_data.GetWorld()->GetAliveEntities();
        }
    }

    ~Query() = default;

    Query(const Query&) = delete;
    Query& operator=(const Query&) = delete;
    Query(Query&&) noexcept = default;
    Query& operator=(Query&&) noexcept = default;

public:
    /** 쿼리 결과가 비어있는지 확인합니다. */
    [[nodiscard]] bool IsEmpty();

    /** 특정 엔티티가 쿼리 조건을 만족하는 경우, 해당 컴포넌트들을 반환합니다. */
    [[nodiscard]] Optional<FetchTypes> Get(Entity entity);

    /** 쿼리 결과가 정확히 하나일 때만 컴포넌트들을 Optional로 반환합니다. 결과가 없거나 여러 개이면 nullopt를 반환합니다. */
    [[nodiscard]] Optional<FetchTypes> GetSingle();

    /** 쿼리 결과가 정확히 하나일 때만 컴포넌트들을 반환합니다. 결과가 없거나 여러 개이면 assert로 프로그램을 중단시킵니다. */
    [[nodiscard]] FetchTypes Single();

private:
    template <typename T>
    static decltype(auto) GetComponentHelper(World* world, Entity entity);

public:
    class Iterator
    {
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type = FetchTypes;
        using difference_type = std::ptrdiff_t;

    public:
        Iterator(Query* self, usize in_index)
            : query_data(&self->query_data)
            , storage_index(in_index)
        {
            if constexpr (HasBasePool)
            {
                iteration_source = query_data->FindSmallestPool();
            }
            else
            {
                iteration_source = &self->alive_entities_cache;
            }
            AdvanceToValid();
        }

        value_type operator*() const noexcept
        {
            World* world = query_data->GetWorld();
            const Entity entity = std::visit([this]<typename Variant>(Variant&& source) -> Entity
            {
                using SourceType = std::decay_t<Variant>;
                if constexpr (std::same_as<SourceType, IStorage*>)
                {
                    return source->GetEntityByIndex(storage_index).Value();
                }
                else // const Array<Entity>*
                {
                    return (*source)[storage_index];
                }
            }, iteration_source);

            // FetchTypes에 명시된 컴포넌트들을 월드에서 가져와 튜플로 묶어 반환
            return utility::WithUnpackedTypes<value_type>([world, entity]<typename... FetchComps>
            {
                return value_type{ GetComponentHelper<FetchComps>(world, entity)... };
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
            return iteration_source == other.iteration_source && storage_index == other.storage_index;
        }

    private:
        void AdvanceToValid()
        {
            ZoneScoped;

            // std::visit를 사용하여 순회 로직을 실행
            std::visit([this]<typename Variant>(Variant&& source)
            {
                using SourceType = std::decay_t<Variant>;
                if constexpr (std::same_as<SourceType, IStorage*>)
                {
                    if (!source) [[unlikely]]
                    {
                        // FindSmallestPool이 nullptr을 반환하는 경우 (예: 해당 컴포넌트를 가진 엔티티가 없음)
                        storage_index = 0;
                        return;
                    }
                    while (storage_index < source->Length())
                    {
                        if (Optional entity_opt = source->GetEntityByIndex(storage_index))
                        {
                            if (query_data->IsEntityValid(*entity_opt))
                            {
                                return;
                            }
                        }
                        ++storage_index;
                    }
                }
                else // const Array<Entity>*
                {
                    assert(source);

                    const auto& entities = *source;
                    while (storage_index < entities.Len())
                    {
                        if (query_data->IsEntityValid(entities[storage_index]))
                        {
                            return;
                        }
                        ++storage_index;
                    }
                }
            }, iteration_source);
        }

    private:
        QueryDataType* query_data;
        usize storage_index;

        std::variant<IStorage*, const Array<Entity>*> iteration_source;
    };

    Iterator begin()
    {
        return Iterator(this, 0);
    }

    Iterator end()
    {
        usize end_index = 0;
        if constexpr (HasBasePool)
        {
            if (auto* pool = query_data.FindSmallestPool())
            {
                end_index = pool->Length();
            }
        }
        else
        {
            end_index = alive_entities_cache.Len();
        }
        return Iterator(this, end_index);
    }

private:
    QueryDataType query_data;
    [[no_unique_address]] CacheType alive_entities_cache;
};

template <typename... Ts> requires QueryParameterPack<Ts...>
bool Query<Ts...>::IsEmpty()
{
    return begin() == end();
}

template <typename... Ts> requires QueryParameterPack<Ts...>
Optional<typename Query<Ts...>::FetchTypes> Query<Ts...>::Get(Entity entity)
{
    if (query_data.IsEntityValid(entity))
    {
        World* world = query_data.GetWorld();
        return utility::WithUnpackedTypes<FetchTypes>([world, entity]<typename... FetchComps>
        {
            return FetchTypes{ GetComponentHelper<FetchComps>(world, entity)... };
        });
    }
    return std::nullopt;
}

template <typename... Ts> requires QueryParameterPack<Ts...>
Optional<typename Query<Ts...>::FetchTypes> Query<Ts...>::GetSingle()
{
    auto it = begin();
    if (it == end())
    {
        return std::nullopt;
    }

    FetchTypes result = *it;
    ++it;
    if (it != end())
    {
        return std::nullopt;
    }

    return result;
}

template <typename... Ts> requires QueryParameterPack<Ts...>
Query<Ts...>::FetchTypes Query<Ts...>::Single()
{
    auto it = begin();
    assert(it != end() && "Called Single() on a query with no matching entities.");

    FetchTypes result = *it;
    ++it;
    assert(it == end() && "Called Single() on a query with more than one matching entity.");

    return result;
}

template <typename... Ts> requires QueryParameterPack<Ts...>
template <typename T>
decltype(auto) Query<Ts...>::GetComponentHelper(World* world, Entity entity)
{
    if constexpr (traits::IsSpecializationOf<T, Optional>)
    {
        static_assert(
            !std::is_reference_v<T>,
            "Optional<T> in a Query must be fetched by value, not by reference."
        );

        using DecayedT = std::decay_t<typename T::ValueType>;
        using WorldType = std::conditional_t<std::is_const_v<typename T::ValueType>, const World*, World*>;

        WorldType world_ptr = world;
        return world_ptr->template TryGetComponent<DecayedT>(entity);
    }
    else if constexpr (std::same_as<std::decay_t<T>, Entity>)
    {
        static_assert(
            !std::is_reference_v<T>,
            "Entity in a Query must be fetched by value, not by reference."
        );
        return entity;
    }
    else
    {
        return world->GetComponent<std::decay_t<T>>(entity);
    }
}
}
