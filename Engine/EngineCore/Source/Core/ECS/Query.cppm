module;
#include "tracy/Tracy.hpp"
export module SE.Core:ECS.Query;
import :ECS.World;
import :ECS.QueryData;

import SE.Types;
import SE.Traits;
import SE.Utility;
import std;

using namespace se::traits::type_traits;
using namespace se::utility::type;

namespace se::core::ecs
{
/**
 * 조건에 맞는 엔티티와 컴포넌트들을 순회(iterate)하기 위한 인터페이스입니다.
 * @tparam Ts 조회할 컴포넌트 타입들
 */
export template <typename... Ts>
    requires (sizeof...(Ts) > 0)
    && TupleHasUniqueTypes<FlattenTuple<std::tuple<Ts...>>>
class Query
{
    friend class Iterator;
    using QueryDataType = QueryData<Ts...>;

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
    class Iterator
    {
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type = QueryDataType::FetchTypes;
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
                return value_type{ world->GetComponent<std::decay_t<FetchComps>>(entity)... };
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