#pragma once

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Container/HashSet.h"
#include "SimpleEngine/Core/Container/Optional.h"
#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/Reflection/TypeId.h"
#include "SimpleEngine/Core/Reflection/TypeInfo.h"
#include "SimpleEngine/Core/Reflection/TypeName.h"
#include "SimpleEngine/Core/Reflection/TypeRegistry.h"
#include "SimpleEngine/Core/Types/Guid.h"
#include "SimpleEngine/Traits/ContainerTraits.h"
#include "SimpleEngine/Traits/TypeTraits.h"

#include <type_traits>


namespace se
{
// forward declaration
template <typename T>
const TypeInfo& EnsureRegistered();

/**
 * 타입 T에 대한 TypeInfo를 등록하는 구조체
 */
template <typename T>
struct Registrar
{
    /** "Registrar<T>가 특수화됐는지" requires{} 판별용 마커 */
    using UnregisteredMarker = void;

    static void Fill(TypeInfo& info)
    {
        info.size = sizeof(T);
        info.alignment = alignof(T);
        info.name = TypeNameOf<T>();

        if constexpr (std::is_fundamental_v<T>)
        {
            info.shape = OpaqueInfo{};
        }
        else if constexpr (traits::EnumType<T>)
        {
            using Underlying = std::underlying_type_t<T>;
            info.shape = EnumInfo{
                .underlying = TypeId::Of<Underlying>(),
                .entries = {},
            };
            EnsureRegistered<Underlying>();
        }
        else
        {
            static_assert(traits::AlwaysFalse<T>,
                "Registrar<T>: no Fill() implementation found for this type. "
                "For structs/classes, ensure SE_DECLARE_REFLECTION(T) and its registration block are defined.");
        }
    }
};

namespace detail
{
/**
 * Registrar<T>가 primary로 떨어졌는지 확인
 * @note requires{}가 MSVC에서 SFINAE로 안 먹혀서 void_t로 우회
 */
template <typename T, typename = void>
constexpr bool IsRegistrarUnspecialized = false;

template <typename T>
constexpr bool IsRegistrarUnspecialized<T, std::void_t<typename Registrar<T>::UnregisteredMarker>> = true;
} // namespace detail

// ----- 코어 타입 Opaque 등록 -----
#define SE_DEFINE_OPAQUE_REGISTRAR(type) \
template <> \
struct Registrar<type> \
{ \
    static void Fill(TypeInfo& info) \
    { \
        info.size = sizeof(type); \
        info.alignment = alignof(type); \
        info.name = TypeNameOf<type>(); \
        info.shape = OpaqueInfo{}; \
    } \
}

SE_DEFINE_OPAQUE_REGISTRAR(TypeId);
SE_DEFINE_OPAQUE_REGISTRAR(String);
SE_DEFINE_OPAQUE_REGISTRAR(Guid);

#undef SE_DEFINE_OPAQUE_REGISTRAR

/** Array-like 컨테이너(Array, FixedArray 등)의 등록 특수화 */
template <traits::ArrayLike Container>
struct Registrar<Container>
{
    static void Fill(TypeInfo& info)
    {
        using ElementType = traits::InnerOf<Container>;
        info.size = sizeof(Container);
        info.alignment = alignof(Container);
        info.name = TypeNameOf<Container>();
        info.shape = ArrayInfo{
            .element = TypeId::Of<ElementType>(),
        };
        EnsureRegistered<ElementType>(); // 내부 요소 타입도 자동으로 등록 대상에 포함
    }
};

/** Set-like 컨테이너(HashSet, Set, FlatSet 등)의 등록 특수화 */
template <traits::SetLike Container>
struct Registrar<Container>
{
    static void Fill(TypeInfo& info)
    {
        using ElementType = traits::InnerOf<Container>;
        info.size = sizeof(Container);
        info.alignment = alignof(Container);
        info.name = TypeNameOf<Container>();
        info.shape = SetInfo{
            .element = TypeId::Of<ElementType>(),
        };
        EnsureRegistered<ElementType>();
    }
};

/** Map-like 컨테이너(HashMap, Map, FlatMap 등)의 등록 특수화 */
template <traits::MapLike Container>
struct Registrar<Container>
{
    static void Fill(TypeInfo& info)
    {
        using KeyType = traits::KeyOf<Container>;
        using ValueType = traits::ValueOf<Container>;
        info.size = sizeof(Container);
        info.alignment = alignof(Container);
        info.name = TypeNameOf<Container>();
        info.shape = MapInfo{
            .key = TypeId::Of<KeyType>(),
            .value = TypeId::Of<ValueType>(),
        };
        EnsureRegistered<KeyType>();
        EnsureRegistered<ValueType>();
    }
};

/** Optional<T> (T, T*, T& 세 가지 형태 전부 이 특수화 하나로 매치됩니다) */
template <traits::OptionalLike Container>
struct Registrar<Container>
{
    static void Fill(TypeInfo& info)
    {
        using InnerType = traits::InnerOf<Container>;
        info.size = sizeof(Container);
        info.alignment = alignof(Container);
        info.name = TypeNameOf<Container>();
        info.shape = OptionalInfo{
            .inner = TypeId::Of<InnerType>(),
        };
        EnsureRegistered<InnerType>();
    }
};

/** T가 TypeRegistry에 등록되어 있음을 보장하고, 등록된 TypeInfo를 반환합니다. */
template <typename T>
const TypeInfo& EnsureRegistered()
{
    using CleanType = std::remove_cvref_t<T>;
    static const TypeInfo& info = [] -> const TypeInfo&
    {
        TypeInfo& slot = TypeRegistry::Get().Emplace(TypeId::Of<CleanType>());
        Registrar<CleanType>::Fill(slot);
        return slot;
    }();
    return info;
}
} // namespace se

/** 비침투형(non-intrusive) 타입의 Registrar<T> 특수화를 헤더에 선언합니다. */
#define SE_DECLARE_REFLECTION(type) \
namespace se \
{ \
    template <> \
    struct Registrar<type> \
    { \
        static void Fill(TypeInfo& info); \
    }; \
}
