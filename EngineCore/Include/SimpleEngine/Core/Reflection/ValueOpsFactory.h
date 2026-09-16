#pragma once

#include "SimpleEngine/Core/Reflection/ValueOps.h"
#include "SimpleEngine/Traits/ContainerTraits.h"

#include <memory>
#include <type_traits>
#include <utility>


namespace se::detail
{
template <traits::ArrayLike Container>
ArrayOps MakeArrayOps()
{
    using ElementType = traits::InnerOf<Container>;

    ArrayOps ops{};
    ops.len = [](const void* c) static -> usize { return static_cast<const Container*>(c)->Len(); };
    ops.data = [](void* c) static -> void* { return static_cast<Container*>(c)->Data(); };
    ops.element_at = [](void* c, usize index) static -> void* { return &(*static_cast<Container*>(c))[index]; };

    if constexpr (requires (Container& container, usize count) { container.Resize(count); })
    {
        ops.resize = [](void* c, usize count) static { static_cast<Container*>(c)->Resize(count); };
    }

    if constexpr (std::is_trivially_default_constructible_v<ElementType>
        && requires (Container& container, usize count) { container.ResizeUninitialized(count); })
    {
        ops.resize_uninitialized = [](void* c, usize count) static
        {
            static_cast<Container*>(c)->ResizeUninitialized(count);
        };
    }

    ops.element_trivially_copyable = std::is_trivially_copyable_v<ElementType>;
    ops.element_is_pointer = std::is_pointer_v<ElementType>;
    return ops;
}

template <traits::SetLike Container>
SetOps MakeSetOps()
{
    using ElementType = traits::InnerOf<Container>;

    SetOps ops{};
    ops.len = [](const void* c) static -> usize { return static_cast<const Container*>(c)->Len(); };
    ops.clear = [](void* c) static { static_cast<Container*>(c)->Clear(); };

    if constexpr (traits::Reservable<Container>)
    {
        ops.reserve = [](void* c, usize count) static { static_cast<Container*>(c)->Reserve(count); };
    }

    if constexpr (std::is_move_constructible_v<ElementType>)
    {
        ops.emplace_moved = [](void* c, void* element) static
        {
            static_cast<Container*>(c)->Emplace(std::move(*static_cast<ElementType*>(element)));
        };
    }

    ops.for_each = [](void* c, void (*visit)(const void*, void*), void* user_data) static
    {
        for (const auto& element : *static_cast<Container*>(c))
        {
            visit(&element, user_data);
        }
    };

    return ops;
}

template <traits::MapLike Container>
MapOps MakeMapOps()
{
    using KeyType = traits::KeyOf<Container>;
    using ValueType = traits::ValueOf<Container>;

    MapOps ops{};
    ops.len = [](const void* c) static -> usize { return static_cast<const Container*>(c)->Len(); };
    ops.clear = [](void* c) static { static_cast<Container*>(c)->Clear(); };

    if constexpr (traits::Reservable<Container>)
    {
        ops.reserve = [](void* c, usize count) static { static_cast<Container*>(c)->Reserve(count); };
    }

    if constexpr (std::is_move_constructible_v<KeyType> && std::is_move_constructible_v<ValueType>)
    {
        ops.emplace_moved = [](void* c, void* key, void* value) static
        {
            static_cast<Container*>(c)->Emplace(
                std::move(*static_cast<KeyType*>(key)),
                std::move(*static_cast<ValueType*>(value))
            );
        };
    }

    ops.for_each = [](void* c, void (*visit)(const void*, void*, void*), void* user_data) static
    {
        for (auto&& [key, value] : *static_cast<Container*>(c))
        {
            visit(&key, &value, user_data);
        }
    };

    return ops;
}

template <traits::OptionalLike Container>
OptionalOps MakeOptionalOps()
{
    OptionalOps ops{};
    ops.has_value = [](const void* opt) static -> bool { return static_cast<const Container*>(opt)->HasValue(); };
    ops.value = [](void* opt) static -> void* { return &static_cast<Container*>(opt)->Value(); };
    ops.reset = [](void* opt) static { static_cast<Container*>(opt)->Reset(); };

    // Optional<T&>는 Emplace()가 없음
    if constexpr (requires (Container& optional) { optional.Emplace(); })
    {
        ops.emplace = [](void* opt) static -> void* { return &static_cast<Container*>(opt)->Emplace(); };
    }

    return ops;
}

/** 타입 T의 값 연산 테이블을 만듭니다. */
template <typename T>
ValueOps MakeValueOps()
{
    ValueOps ops{};

    if constexpr (std::is_default_constructible_v<T>)
    {
        ops.default_construct_at = [](void* storage) static { std::construct_at(static_cast<T*>(storage)); };
    }
    if constexpr (std::is_destructible_v<T>)
    {
        ops.destruct_at = [](void* object) static { std::destroy_at(static_cast<T*>(object)); };
    }

    if constexpr (traits::ArrayLike<T>)
    {
        ops.shape_ops = MakeArrayOps<T>();
    }
    else if constexpr (traits::SetLike<T>)
    {
        ops.shape_ops = MakeSetOps<T>();
    }
    else if constexpr (traits::MapLike<T>)
    {
        ops.shape_ops = MakeMapOps<T>();
    }
    else if constexpr (traits::OptionalLike<T>)
    {
        ops.shape_ops = MakeOptionalOps<T>();
    }

    return ops;
}
} // namespace se::detail
