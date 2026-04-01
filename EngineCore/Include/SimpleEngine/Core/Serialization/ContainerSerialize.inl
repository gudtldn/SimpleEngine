#pragma once
#include <type_traits>
#include <utility>


namespace se::detail
{
// Array-like 컨테이너 직렬화
template <traits::ArrayLike Container>
void SerializeArrayContainer(Archive& ar, Container& container)
{
    using ElementType = traits::InnerOf<Container>;

    uint64 count = container.Len();
    ar.BeginArray(count);

    // Resize가 있을 때만 코드 적용 (FixedArray에는 Resize가 없음)
    if constexpr (traits::Resizable<Container>)
    {
        if (ar.IsLoading())
        {
            if constexpr (
                std::is_trivially_default_constructible_v<ElementType>
                && requires { container.ResizeUninitialized(count); }
            )
            {
                container.ResizeUninitialized(count);
            }
            else
            {
                container.Resize(count);
            }
        }
    }

    [&]
    {
        // Binary + trivially_copyable 최적화 (Fast Path)
        if constexpr (std::is_trivially_copyable_v<ElementType> && !std::is_pointer_v<ElementType>)
        {
            if (ar.IsBinary() && count > 0)
            {
                ar << BinaryBlob::FromItems(container.Data(), count);
                return;
            }
        }

        // 일반적인 직렬화 Loop (Slow Path)
        for (uint64 i = 0; i < count; ++i)
        {
            ar << container[i];
        }
    }();

    ar.EndArray();
}

// Set-like 컨테이너 직렬화 (HashSet, Set, FlatSet)
template <traits::SetLike Container>
void SerializeSetContainer(Archive& ar, Container& container)
{
    using ElementType = traits::InnerOf<Container>;

    uint64 count = container.Len();
    ar.BeginArray(count);

    if (ar.IsLoading())
    {
        container.Clear();
        if constexpr (traits::Reservable<Container>)
        {
            container.Reserve(count);
        }

        for (uint64 i = 0; i < count; ++i)
        {
            ElementType value;
            ar << value;
            container.Emplace(std::move(value));
        }
    }
    else
    {
        for (const auto& value : container)
        {
            ar << value;
        }
    }

    ar.EndArray();
}

// Map-like 컨테이너 직렬화 (HashMap, Map, FlatMap)
template <traits::MapLike Container>
void SerializeMapContainer(Archive& ar, Container& container)
{
    using KeyType = traits::KeyOf<Container>;
    using ValueType = traits::ValueOf<Container>;

    uint64 count = container.Len();
    ar.BeginMap(count);

    if (ar.IsLoading())
    {
        container.Clear();
        if constexpr (traits::Reservable<Container>)
        {
            container.Reserve(count);
        }

        for (uint64 i = 0; i < count; ++i)
        {
            KeyType key;
            ar.BeginMapKey();
            ar << key;
            ar.EndMapKey();

            ValueType value;
            ar.BeginMapValue();
            ar << value;
            ar.EndMapValue();

            container.Emplace(std::move(key), std::move(value));
        }
    }
    else
    {
        for (auto& [key, value] : container)
        {
            ar.BeginMapKey();
            ar << key;
            ar.EndMapKey();

            ar.BeginMapValue();
            ar << value;
            ar.EndMapValue();
        }
    }

    ar.EndMap();
}
}  // namespace se::detail
