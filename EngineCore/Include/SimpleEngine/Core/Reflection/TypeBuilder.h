#pragma once
#include <concepts>
#include <type_traits>

#include "SimpleEngine/Core/Reflection/Meta.h"
#include "SimpleEngine/Core/Serialization/Archive.h"
#include "SimpleEngine/Traits/ContainerTraits.h"


namespace se
{
// forward declaration
class TypeRegistry;
SE_CORE_API void AutoSerialize(Archive& ar, const TypeId& type_id, void* instance);

namespace detail
{
/** 멤버 포인터에서 클래스 타입과 멤버 타입을 추출하기 위한 Trait */
template <auto MemberPtr>
struct MemberPointerTraits;

template <typename ClassT, typename MemberT, MemberT ClassT::*Ptr>
struct MemberPointerTraits<Ptr>
{
    using ClassType = ClassT;
    using MemberType = MemberT;
};

/**
 * 특정 타입 T에 대한 리플렉션 정보를 생성하는 Builder
 * @tparam T 등록하려는 대상 타입 (클래스, 구조체 등)
 */
template <typename T>
class TypeBuilder
{
public:
    explicit TypeBuilder(TypeInfo* info, ETypeKind kind)
        : info_ptr(info)
    {
        // info가 어떤 타입 종류인지 설정
        info_ptr->kind = kind;

        // Super가 존재하면 Info에 부모의 TypeId를 저장
        if constexpr (requires{ typename T::Super; })
        {
            using Super = T::Super;
            if constexpr (!std::is_void_v<Super>)
            {
                static_assert(!std::same_as<std::decay_t<T>, std::decay_t<Super>>, "Class cannot inherit from itself!");
                info_ptr->base_or_inner_id = TypeId::Get<Super>();
            }
        }

        // CTOR & DTOR 등록
        if constexpr (std::is_default_constructible_v<T>)
        {
            info_ptr->constructor = []() static -> void* { return new T(); };
        }
        info_ptr->destructor = [](void* ptr) static { delete static_cast<T*>(ptr); };
    }

    ~TypeBuilder()
    {
        // 체이닝 종료 후 serialize 콜백이 미등록된 Struct에 AutoSerialize를 자동 연결합니다.
        if (info_ptr && !info_ptr->serialize && info_ptr->kind == ETypeKind::Struct)
        {
            info_ptr->serialize = [](Archive& ar, void* instance) static
            {
                AutoSerialize(ar, TypeId::Get<T>(), instance);
            };
        }
    }

public:
    /** 타입의 특성 플래그(예: Abstract, Transient 등)를 추가합니다. */
    TypeBuilder& AddFlags(BitFlags<ETypeFlags> flags)
    {
        info_ptr->flags |= flags;
        return *this;
    }

    /**
     * 현재 타입(T)이 구현하는 인터페이스 목록을 등록합니다.
     * @tparam InterfaceTypes T가 상속받는 하나 이상의 인터페이스 타입들
     */
    template <typename... InterfaceTypes>
        requires (std::derived_from<T, InterfaceTypes> && ...)
    TypeBuilder& Implements()
    {
        (ImplementInterface<InterfaceTypes>(), ...);
        return *this;
    }

    /**
     * 멤버 변수(Property)를 리플렉션 시스템에 등록합니다.
     * @tparam MemberPtr 등록할 멤버의 주소 (&MyClass::Value)
     * @param name property의 이름
     */
    template <auto MemberPtr>
    TypeBuilder& Property(StringView name)
    {
        using MemberType = MemberPointerTraits<MemberPtr>::MemberType;

        PropertyInfo prop;
        prop.type_id = TypeId::Get<MemberType>();
        prop.name = name;
        prop.size = sizeof(MemberType);

        // 오프셋 계산
        prop.offset = [] static -> usize
        {
            alignas(T) uint8 dummy[sizeof(T)];
            T* obj_ptr = reinterpret_cast<T*>(dummy);
            return reinterpret_cast<usize>(&(obj_ptr->*MemberPtr)) - reinterpret_cast<usize>(obj_ptr);
        }();

        // 접근자(Accessor) 생성
        prop.accessor = {
            .get_ptr = [](void* instance) static -> void*
            {
                SE_ASSERT(instance, "Instance pointer is null in get_ptr");

                T* typed = static_cast<T*>(instance);
                return &(typed->*MemberPtr);
            },
            .getter = [](const void* instance, void* out_value) static
            {
                SE_ASSERT(instance, "Instance pointer is null in getter");
                SE_ASSERT(out_value, "Target buffer (out_value) is null in getter");

                const T* typed = static_cast<const T*>(instance);
                MemberType* out = static_cast<MemberType*>(out_value);
                *out = typed->*MemberPtr;
            },
            .setter = [](void* instance, const void* in_value) static
            {
                SE_ASSERT(instance, "Instance pointer is null in setter");
                SE_ASSERT(in_value, "Source value (in_value) is null in setter");

                T* typed = static_cast<T*>(instance);
                const MemberType* in = static_cast<const MemberType*>(in_value);
                typed->*MemberPtr = *in;
            },
        };

        prop.serialize = [](Archive& ar, void* ptr) static
        {
            ar << *static_cast<MemberType*>(ptr);
        };

        // 컨테이너 타입 감지 및 ContainerOps 자동 생성
        if constexpr (traits::ArrayLike<MemberType>)
        {
            using ElemType = traits::ElementOf<MemberType>;
            static constexpr ContainerOps ops = MakeArrayOps<MemberType, ElemType>();
            prop.container_ops = &ops;
        }
        else if constexpr (traits::SetLike<MemberType>)
        {
            using ElemType = traits::ElementOf<MemberType>;
            static constexpr ContainerOps ops = MakeSetOps<MemberType, ElemType>();
            prop.container_ops = &ops;
        }
        else if constexpr (traits::MapLike<MemberType>)
        {
            using KeyType = traits::KeyOf<MemberType>;
            using ValType = traits::ValueOf<MemberType>;
            static constexpr ContainerOps ops = MakeMapOps<MemberType, KeyType, ValType>();
            prop.container_ops = &ops;
        }

        info_ptr->properties.Push(prop);
        return *this;
    }

    /**
     * 방금 등록된 프로퍼티에 메타데이터를 적용합니다.
     * 매크로에서 생성된 PropertyMetadata 구조체를 받아 병합합니다.
     */
    TypeBuilder& ApplyMetadata(const PropertyMetadata& meta)
    {
        SE_ASSERT(!info_ptr->properties.IsEmpty(), "No property registered yet! Call Property() before ApplyMetadata().");

        PropertyInfo& last_prop = info_ptr->properties.Back().Value();
        last_prop.metadata = meta;

        return *this;
    }

public:
    /** 외부에서 정의한 직렬화 로직을 연결합니다. */
    TypeBuilder& Serialize(TypeInfo::SerializeFunc func)
    {
        info_ptr->serialize = func;
        return *this;
    }

    /** Enum 항목 목록 접근자를 연결합니다. (SE_REFLECT_ENUM 전용) */
    TypeBuilder& EnumEntries(TypeInfo::EnumEntriesFunc func)
    {
        info_ptr->enum_entries = func;
        return *this;
    }

private:
    template <typename InterfaceType>
        requires std::derived_from<T, InterfaceType>
    // ReSharper disable once CppMemberFunctionMayBeConst
    void ImplementInterface()
    {
        constexpr TypeId type_id = TypeId::Get<InterfaceType>();
        info_ptr->interfaces.Insert(type_id, InterfaceInfo{
            .type_id = type_id,
            .caster = [](void* instance) static -> void*
            {
                // 주소 보정 (Pointer Adjustment)
                T* typed = static_cast<T*>(instance);
                return static_cast<InterfaceType*>(typed);
            }
        });
    }

    /** Array-like 컨테이너의 ContainerOps 생성 */
    template <typename Container, typename ElemType>
    static constexpr ContainerOps MakeArrayOps()
    {
        ContainerOps ops;
        ops.kind = EContainerKind::Array;
        ops.element_type_id = TypeId::Get<ElemType>();

        ops.size = [](const void* c) static -> usize
        {
            return static_cast<const Container*>(c)->Len();
        };

        ops.clear = [](void* c) static
        {
            static_cast<Container*>(c)->Clear();
        };

        if constexpr (std::is_default_constructible_v<ElemType>)
        {
            ops.add = [](void* c) static
            {
                static_cast<Container*>(c)->Emplace();
            };
        }

        ops.remove_at = [](void* c, usize index) static
        {
            static_cast<Container*>(c)->RemoveAt(index);
        };

        ops.for_each = [](void* c, bool (*callback)(usize idx, void* key_or_elem, void* value_or_null, void* user_data), void* user) static
        {
            auto& container = *static_cast<Container*>(c);
            usize idx = 0;
            for (auto& elem : container)
            {
                if (!callback(idx, &elem, nullptr, user))
                {
                    break;
                }
                ++idx;
            }
        };

        return ops;
    }

    /** Set-like 컨테이너의 ContainerOps 생성 */
    template <typename Container, typename ElemType>
    static constexpr ContainerOps MakeSetOps()
    {
        ContainerOps ops;
        ops.kind = EContainerKind::Set;
        ops.element_type_id = TypeId::Get<ElemType>();

        ops.size = [](const void* c) static -> usize
        {
            return static_cast<const Container*>(c)->Len();
        };

        ops.clear = [](void* c) static
        {
            static_cast<Container*>(c)->Clear();
        };

        if constexpr (std::is_default_constructible_v<ElemType>)
        {
            ops.add = [](void* c) static
            {
                static_cast<Container*>(c)->Emplace();
            };
        }

        ops.remove_at = [](void* c, usize target_idx) static
        {
            auto& container = *static_cast<Container*>(c);
            usize idx = 0;
            for (auto it = container.begin(); it != container.end(); ++it, ++idx)
            {
                if (idx == target_idx)
                {
                    container.Remove(*it);
                    return;
                }
            }
        };

        ops.for_each = [](void* c, bool(*callback)(usize, void*, void*, void*), void* user) static
        {
            auto& container = *static_cast<Container*>(c);
            usize idx = 0;
            for (auto& elem : container)
            {
                // Set의 요소는 const이므로 표시 전용으로 const_cast
                if (!callback(idx, const_cast<ElemType*>(&elem), nullptr, user))
                {
                    break;
                }
                ++idx;
            }
        };

        return ops;
    }

    /** Map-like 컨테이너의 ContainerOps 생성 */
    template <typename Container, typename KeyType, typename ValType>
    static constexpr ContainerOps MakeMapOps()
    {
        ContainerOps ops;
        ops.kind = EContainerKind::Map;
        ops.element_type_id = TypeId::Get<KeyType>();
        ops.value_type_id = TypeId::Get<ValType>();

        ops.size = [](const void* c) static -> usize
        {
            return static_cast<const Container*>(c)->Len();
        };

        ops.clear = [](void* c) static
        {
            static_cast<Container*>(c)->Clear();
        };

        if constexpr (std::is_default_constructible_v<KeyType> && std::is_default_constructible_v<ValType>)
        {
            ops.add = [](void* c) static
            {
                static_cast<Container*>(c)->Emplace(KeyType{});
            };
        }

        ops.remove_at = [](void* c, usize target_idx) static
        {
            auto& container = *static_cast<Container*>(c);
            usize idx = 0;
            for (auto it = container.begin(); it != container.end(); ++it, ++idx)
            {
                if (idx == target_idx)
                {
                    container.Remove(it->first);
                    return;
                }
            }
        };

        ops.for_each = [](void* c, bool(*callback)(usize, void*, void*, void*), void* user) static
        {
            auto& container = *static_cast<Container*>(c);
            usize idx = 0;
            for (auto& [key, value] : container)
            {
                // Map의 key는 const이므로 표시 전용으로 const_cast
                if (!callback(idx, const_cast<KeyType*>(&key), &value, user))
                {
                    break;
                }
                ++idx;
            }
        };

        return ops;
    }

private:
    TypeInfo* info_ptr;
};
} // namespace detail
} // namespace se
