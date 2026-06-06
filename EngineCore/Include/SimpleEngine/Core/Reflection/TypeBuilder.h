#pragma once

#include "SimpleEngine/Core/Reflection/Meta.h"
#include "SimpleEngine/Core/Serialization/Archive.h"
#include "SimpleEngine/Traits/ContainerTraits.h"

#include <concepts>
#include <type_traits>


namespace se
{
// forward declaration
class TypeRegistry;

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
                info_ptr->base_or_inner_id = TypeId::Of<Super>();
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
                AutoSerialize(ar, TypeId::Of<T>(), instance);
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

    /** ECS 타입 분류를 설정합니다. */
    TypeBuilder& SetECSKind(EECSKind kind)
    {
        info_ptr->ecs_kind = kind;
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
        prop.type_id = TypeId::Of<MemberType>();
        prop.name = name;
        prop.size = sizeof(MemberType);

        // 오프셋 계산
        prop.offset = [] static -> usize
        {
            alignas(T) u8 dummy[sizeof(T)];
            T* obj_ptr = reinterpret_cast<T*>(dummy);
            return reinterpret_cast<usize>(&(obj_ptr->*MemberPtr)) - reinterpret_cast<usize>(obj_ptr);
        }();

        // 접근자(Accessor) 생성
        prop.accessor = {
            .get = [](const void* instance) static -> const void*
            {
                SE_ASSERT(instance, "Instance pointer is null in get");
                return &(static_cast<const T*>(instance)->*MemberPtr);
            },
            .get_mut = [](void* instance) static -> void*
            {
                SE_ASSERT(instance, "Instance pointer is null in get_mut");
                return &(static_cast<T*>(instance)->*MemberPtr);
            },
            .set = [](void* instance, const void* in_value) static
            {
                SE_ASSERT(instance, "Instance pointer is null in set");
                SE_ASSERT(in_value, "Source value (in_value) is null in set");

                if constexpr (std::is_copy_assignable_v<MemberType>)
                {
                    static_cast<T*>(instance)->*MemberPtr = *static_cast<const MemberType*>(in_value);
                }
                else
                {
                    SE_ASSERT(false, "This property is not copy-assignable.");
                }
            },
        };

        prop.serialize = [](Archive& ar, void* ptr) static
        {
            ar << *static_cast<MemberType*>(ptr);
        };

        // 컨테이너 타입 감지 및 ContainerOps 자동 생성
        if constexpr (traits::ArrayLike<MemberType>)
        {
            using ElemType = traits::InnerOf<MemberType>;
            static constexpr ContainerOps ops = MakeArrayOps<MemberType, ElemType>();
            prop.container_ops = &ops;
        }
        else if constexpr (traits::SetLike<MemberType>)
        {
            using ElemType = traits::InnerOf<MemberType>;
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
        // Optional 타입 감지 및 OptionalOps 자동 생성
        else if constexpr (traits::OptionalLike<MemberType>)
        {
            using InnerType = traits::InnerOf<MemberType>;
            static constexpr OptionalOps ops = MakeOptionalOps<MemberType, InnerType>();
            prop.optional_ops = &ops;
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
    /** Enum 항목 목록 접근자를 연결합니다. (SE_REFLECT_ENUM 전용) */
    TypeBuilder& EnumEntries(TypeInfo::EnumEntriesFunc func)
    {
        info_ptr->enum_entries = func;
        return *this;
    }

public:
    /** 외부에서 정의한 직렬화 로직을 연결합니다. */
    TypeBuilder& Serialize(TypeInfo::SerializeFunc func)
    {
        info_ptr->serialize = func;
        return *this;
    }

private:
    template <typename InterfaceType>
        requires std::derived_from<T, InterfaceType>
    // ReSharper disable once CppMemberFunctionMayBeConst
    void ImplementInterface()
    {
        constexpr TypeId type_id = TypeId::Of<InterfaceType>();
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

    // ============================================================================
    // 중첩 타입의 Ops를 생성하는 Helper 함수
    // ============================================================================

    /** 요소 타입이 컨테이너인 경우 중첩 ContainerOps 포인터를 반환합니다. */
    template <typename ElemType>
    static constexpr const ContainerOps* GetNestedContainerOps()
    {
        if constexpr (traits::ArrayLike<ElemType>)
        {
            using Inner = traits::InnerOf<ElemType>;
            static constexpr ContainerOps nested = MakeArrayOps<ElemType, Inner>();
            return &nested;
        }
        else if constexpr (traits::SetLike<ElemType>)
        {
            using Inner = traits::InnerOf<ElemType>;
            static constexpr ContainerOps nested = MakeSetOps<ElemType, Inner>();
            return &nested;
        }
        else if constexpr (traits::MapLike<ElemType>)
        {
            using K = traits::KeyOf<ElemType>;
            using V = traits::ValueOf<ElemType>;
            static constexpr ContainerOps nested = MakeMapOps<ElemType, K, V>();
            return &nested;
        }
        else
        {
            return nullptr;
        }
    }

    /** 요소 타입이 Optional인 경우 중첩 OptionalOps 포인터를 반환합니다. */
    template <typename ElemType>
    static constexpr const OptionalOps* GetNestedOptionalOps()
    {
        if constexpr (traits::OptionalLike<ElemType>)
        {
            using Inner = traits::InnerOf<ElemType>;
            static constexpr OptionalOps nested = MakeOptionalOps<ElemType, Inner>();
            return &nested;
        }
        else
        {
            return nullptr;
        }
    }

    // ============================================================================
    // ContainerOps / OptionalOps 팩토리 함수
    // ============================================================================

    /** Array-like 컨테이너의 ContainerOps 생성 */
    template <typename Container, typename ElemType>
    static constexpr ContainerOps MakeArrayOps()
    {
        ContainerOps ops;
        ops.kind = EContainerKind::Array;
        ops.element_type_id = TypeId::Of<ElemType>();

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

        // 요소 타입의 중첩 ops
        ops.element_container_ops = GetNestedContainerOps<ElemType>();
        ops.element_optional_ops  = GetNestedOptionalOps<ElemType>();

        return ops;
    }

    /** Set-like 컨테이너의 ContainerOps 생성 */
    template <typename Container, typename ElemType>
    static constexpr ContainerOps MakeSetOps()
    {
        ContainerOps ops;
        ops.kind = EContainerKind::Set;
        ops.element_type_id = TypeId::Of<ElemType>();

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

        // 요소 타입의 중첩 ops
        ops.element_container_ops = GetNestedContainerOps<ElemType>();
        ops.element_optional_ops  = GetNestedOptionalOps<ElemType>();

        return ops;
    }

    /** Map-like 컨테이너의 ContainerOps 생성 */
    template <typename Container, typename KeyType, typename ValType>
    static constexpr ContainerOps MakeMapOps()
    {
        ContainerOps ops;
        ops.kind = EContainerKind::Map;
        ops.element_type_id = TypeId::Of<KeyType>();
        ops.value_type_id = TypeId::Of<ValType>();

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
            for (auto&& [key, value] : container)
            {
                // Map의 key는 const이므로 표시 전용으로 const_cast
                if (!callback(idx, const_cast<KeyType*>(&key), &value, user))
                {
                    break;
                }
                ++idx;
            }
        };

        // Key(element)의 중첩 ops
        ops.element_container_ops = GetNestedContainerOps<KeyType>();
        ops.element_optional_ops  = GetNestedOptionalOps<KeyType>();

        // Value의 중첩 ops
        ops.value_container_ops = GetNestedContainerOps<ValType>();
        ops.value_optional_ops  = GetNestedOptionalOps<ValType>();

        return ops;
    }

    /** Optional 타입의 OptionalOps 생성 */
    template <typename Opt, typename InnerType>
    static constexpr OptionalOps MakeOptionalOps()
    {
        OptionalOps ops;
        ops.inner_type_id = TypeId::Of<InnerType>();

        ops.has_value = [](const void* o) static -> bool
        {
            return static_cast<const Opt*>(o)->HasValue();
        };

        ops.get_value = [](void* o) static -> void*
        {
            return &static_cast<Opt*>(o)->Value();
        };

        ops.reset = [](void* o) static
        {
            static_cast<Opt*>(o)->Reset();
        };

        if constexpr (std::is_default_constructible_v<InnerType>)
        {
            ops.emplace_default = [](void* o) static
            {
                static_cast<Opt*>(o)->Emplace();
            };
        }

        // 내부 타입의 중첩 ops
        ops.inner_container_ops = GetNestedContainerOps<InnerType>();
        ops.inner_optional_ops  = GetNestedOptionalOps<InnerType>();

        return ops;
    }

private:
    TypeInfo* info_ptr;
};
} // namespace detail
} // namespace se
