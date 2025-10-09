module;
#include "tracy/Tracy.hpp"
export module SE.Core:ECS.Query;
import :ECS.World;
import :ECS.QueryData;
import :ECS.QueryConcepts;

import SE.Types;
import SE.Traits;
import SE.Utility;
import std;

import <cassert>;

using namespace se::traits::type_traits;
using namespace se::utility::type;


namespace se::core::ecs
{
/**
 * 조건에 맞는 엔티티와 컴포넌트들을 순회(iterate)하기 위한 인터페이스입니다.
 * @tparam Ts 조회할 컴포넌트 타입들
 */
export template <typename... Ts>
    requires QueryParameterPack<Ts...>
class Query
{
    friend class Iterator;
    using QueryDataType = QueryData<Ts...>;
    using FetchTypes = QueryDataType::FetchTypes;

public:
    explicit Query(World* in_world)
        : query_data(in_world)
    {
    }

    ~Query() = default;

    Query(const Query&) = default;
    Query& operator=(const Query&) = default;
    Query(Query&&) = default;
    Query& operator=(Query&&) = default;

public:
    /** 쿼리 결과가 비어있는지 확인합니다. */
    [[nodiscard]] bool IsEmpty()
    {
        return begin() == end();
    }

    /** 특정 엔티티가 쿼리 조건을 만족하는 경우, 해당 컴포넌트들을 반환합니다. */
    [[nodiscard]] Optional<FetchTypes> Get(Entity entity)
    {
        if (query_data.IsEntityValid(entity))
        {
            World* world = query_data.GetWorld();
            return utility::type::WithUnpackedTypes<FetchTypes>([world, entity]<typename... FetchComps>
            {
                return FetchTypes{ GetComponentHelper<FetchComps>(world, entity)... };
            });
        }
        return std::nullopt;
    }

    /** 쿼리 결과가 정확히 하나일 때만 컴포넌트들을 Optional로 반환합니다. 결과가 없거나 여러 개이면 nullopt를 반환합니다. */
    [[nodiscard]] Optional<FetchTypes> GetSingle()
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

    /** 쿼리 결과가 정확히 하나일 때만 컴포넌트들을 반환합니다. 결과가 없거나 여러 개이면 assert로 프로그램을 중단시킵니다. */
    [[nodiscard]] FetchTypes Single()
    {
        auto it = begin();
        assert(it != end() && "Called Single() on a query with no matching entities.");

        FetchTypes result = *it;
        ++it;
        assert(it == end() && "Called Single() on a query with more than one matching entity.");

        return result;
    }

private:
    template <typename T>
    static decltype(auto) GetComponentHelper(World* world, Entity entity)
    {
        if constexpr (IsSpecializationOf<T, Optional>)
        {
            using DecayedT = std::decay_t<typename T::InnerType>;
            return world->TryGetComponent<DecayedT>(entity);
        }
        else
        {
            using DecayedT = std::decay_t<T>;
            return world->GetComponent<DecayedT>(entity);
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
        Iterator(QueryDataType* in_query_data, IStorage* in_pool, size_t in_index)
            : query_data(in_query_data)
            , base_pool(in_pool)
            , storage_index(in_index)
        {
            AdvanceToValid();
        }

        value_type operator*() const noexcept
        {
            World* world = query_data->GetWorld();
            Entity entity = *base_pool->GetEntityByIndex(storage_index);

            // FetchTypes에 명시된 컴포넌트들을 월드에서 가져와 튜플로 묶어 반환
            return utility::type::WithUnpackedTypes<value_type>([world, entity]<typename... FetchComps>
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
            return storage_index == other.storage_index && base_pool == other.base_pool;
        }

    private:
        void AdvanceToValid()
        {
            ZoneScoped;

            if (!base_pool || storage_index >= base_pool->Length())
            {
                return;
            }

            while (storage_index < base_pool->Length())
            {
                if (Optional<Entity> entity_opt = base_pool->GetEntityByIndex(storage_index))
                {
                    if (query_data->IsEntityValid(*entity_opt))
                    {
                        return;
                    }
                }
                ++storage_index;
            }
        }

    private:
        QueryDataType* query_data;
        IStorage* base_pool;
        size_t storage_index;
    };

    Iterator begin()
    {
        IStorage* smallest_pool = query_data.FindSmallestPool();
        return Iterator(&query_data, smallest_pool, 0);
    }

    Iterator end()
    {
        IStorage* smallest_pool = query_data.FindSmallestPool();
        const size_t end_index = smallest_pool ? smallest_pool->Length() : 0;
        return Iterator(&query_data, smallest_pool, end_index);
    }

private:
    QueryDataType query_data;
};
}
