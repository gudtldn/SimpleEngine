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
    using TargetWorld = QueryDataType::TargetWorld;

    static constexpr bool HasBasePool = std::tuple_size_v<typename QueryDataType::PredicateTypes> > 0;
    using IterationSourceType = std::conditional_t<HasBasePool, const IStorage*, const Array<Entity>*>;

public:
    explicit Query(TargetWorld& in_world)
        : query_data(in_world)
    {
        if constexpr (HasBasePool)
        {
            iteration_source = query_data.FindSmallestPool();
        }
        else
        {
            iteration_source = &in_world.GetAliveEntities();
        }
    }

    ~Query() = default;

    Query(const Query&) = delete;
    Query& operator=(const Query&) = delete;
    Query(Query&&) noexcept = default;
    Query& operator=(Query&&) noexcept = default;

public:
    /** 쿼리 결과가 비어있는지 확인합니다. */
    [[nodiscard]] bool IsEmpty() const
    {
        return begin() == end();
    }

    /** 특정 엔티티가 쿼리 조건을 만족하는 경우, 해당 컴포넌트들을 반환합니다. */
    [[nodiscard]] Optional<FetchTypes> TryGet(Entity entity) const
    {
        if (query_data.IsEntityValid(entity))
        {
            TargetWorld& world = query_data.GetWorld();
            return traits::ApplyTypes<FetchTypes>([&world, entity]<typename... FetchComps>
            {
                return FetchTypes{ Fetch<FetchComps>(world, entity)... };
            });
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
        SE_ASSERT(it != end(), "Called GetSingle() on a query with no matching entities.");

        FetchTypes result = *it;
        ++it;
        SE_ASSERT(it == end(), "Called GetSingle() on a query with more than one matching entity.");

        return result;
    }

private:
    template <typename T>
    static decltype(auto) Fetch(TargetWorld& world, Entity entity)
    {
        using RawType = std::remove_cvref_t<T>;

        if constexpr (traits::OptionalLike<RawType>)
        {
            static_assert(
                !std::is_reference_v<T>,
                "Optional<T> in a Query must be fetched by value, not by reference."
            );

            using InnerType = std::remove_cvref_t<traits::InnerOf<RawType>>;
            return world.template TryGetComponent<InnerType>(entity);
        }
        else if constexpr (std::same_as<RawType, Entity>)
        {
            static_assert(
                !std::is_reference_v<T>,
                "Entity in a Query must be fetched by value, not by reference."
            );
            return entity;
        }
        else
        {
            return world.template GetComponent<RawType>(entity);
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
        Iterator(const Query* self, usize in_index)
            : query_data(&self->query_data)
            , storage_index(in_index)
            , iteration_source(self->iteration_source)
        {
            AdvanceToValid();
        }

        value_type operator*() const noexcept
        {
            TargetWorld& world = query_data->GetWorld();
            Entity entity;
            if constexpr (HasBasePool)
            {
                entity = iteration_source->GetEntityByIndex(storage_index).Value();
            }
            else
            {
                entity = (*iteration_source)[storage_index];
            }

            // FetchTypes에 명시된 컴포넌트들을 월드에서 가져와 튜플로 묶어 반환
            return traits::ApplyTypes<value_type>([&world, entity]<typename... FetchComps>
            {
                return value_type{ Fetch<FetchComps>(world, entity)... };
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
            if constexpr (HasBasePool)
            {
                if (!iteration_source) [[unlikely]]
                {
                    // FindSmallestPool이 nullptr을 반환하는 경우 (예: 해당 컴포넌트를 가진 엔티티가 없음)
                    storage_index = 0;
                    return;
                }
                while (storage_index < iteration_source->Len())
                {
                    if (Optional entity_opt = iteration_source->GetEntityByIndex(storage_index))
                    {
                        if (query_data->IsEntityValid(*entity_opt))
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

                const auto& entities = *iteration_source;
                while (storage_index < entities.Len())
                {
                    if (query_data->IsEntityValid(entities[storage_index]))
                    {
                        return;
                    }
                    ++storage_index;
                }
            }
        }

    private:
        const QueryDataType* query_data;
        usize storage_index;
        IterationSourceType iteration_source;
    };

    [[nodiscard]] Iterator begin() const
    {
        return Iterator{ this, 0 };
    }

    [[nodiscard]] Iterator end() const
    {
        usize end_index = 0;
        if constexpr (HasBasePool)
        {
            if (iteration_source)
            {
                end_index = iteration_source->Len();
            }
        }
        else
        {
            end_index = iteration_source->Len();
        }
        return Iterator{ this, end_index };
    }

private:
    QueryDataType query_data;
    IterationSourceType iteration_source;
};
} // namespace se
