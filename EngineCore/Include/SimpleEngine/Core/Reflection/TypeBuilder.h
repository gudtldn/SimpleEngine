#pragma once
#include <concepts>
#include <type_traits>

#include "SimpleEngine/Core/Reflection/Meta.h"


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
    explicit TypeBuilder(TypeInfo* info)
        : info_ptr(info)
    {
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

public:
    /** 타입의 특성 플래그(예: Abstract, Transient 등)를 추가합니다. */
    TypeBuilder& AddFlags(BitFlags<ETypeFlags> flags)
    {
        info_ptr->flags |= flags;
        return *this;
    }

    /** 타입의 종류(Struct, Class, Enum 등) 설정합니다. */
    TypeBuilder& SetKind(ETypeKind kind)
    {
        info_ptr->kind = kind;
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
        prop.offset = reinterpret_cast<usize>(&(static_cast<T*>(nullptr)->*MemberPtr));

        // 접근자(Accessor) 생성
        prop.accessor.get_ptr = [](void* instance) static -> void*
        {
            SE_ASSERT(instance, "Instance pointer is null in get_ptr");

            T* typed = static_cast<T*>(instance);
            return &(typed->*MemberPtr);
        };

        prop.accessor.getter = [](const void* instance, void* out_value) static
        {
            SE_ASSERT(instance, "Instance pointer is null in getter");
            SE_ASSERT(out_value, "Target buffer (out_value) is null in getter");

            const T* typed = static_cast<const T*>(instance);
            MemberType* out = static_cast<MemberType*>(out_value);
            *out = typed->*MemberPtr;
        };

        prop.accessor.setter = [](void* instance, const void* in_value) static
        {
            SE_ASSERT(instance, "Instance pointer is null in setter");
            SE_ASSERT(in_value, "Source value (in_value) is null in setter");

            T* typed = static_cast<T*>(instance);
            const MemberType* in = static_cast<const MemberType*>(in_value);
            typed->*MemberPtr = *in;
        };

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

    /** 외부에서 정의한 UI 렌더링 로직을 연결합니다. */
    TypeBuilder& DrawUI(TypeInfo::DrawUIFunc func)
    {
        info_ptr->draw_ui = func;
        return *this;
    }

private:
    TypeInfo* info_ptr;
};
} // namespace detail
} // namespace se
