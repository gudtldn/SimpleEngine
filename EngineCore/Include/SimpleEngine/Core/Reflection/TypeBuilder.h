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
    TypeBuilder& AddFlags(ETypeFlags flags)
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

    // /**
    //  * 부모 클래스 관계를 등록합니다.
    //  * @tparam BaseType T가 상속받는 부모 클래스
    //  */
    // template <typename BaseType>
    //     requires std::derived_from<T, BaseType>
    // TypeBuilder& Base()
    // {
    //     static_assert(!std::same_as<std::decay_t<T>, std::decay_t<BaseType>>, "Base type cannot be same as self");
    //     info_ptr->base_or_inner_id = TypeId::Get<BaseType>();
    //     return *this;
    // }

    /**
     * 멤버 변수(Property)를 리플렉션 시스템에 등록합니다.
     * @tparam MemberPtr 등록할 멤버의 주소 (&MyClass::Value)
     */
    template <auto MemberPtr>
    TypeBuilder& Property(StringView name, StringView display_name = "", StringView category = "Default")
    {
        using MemberType = MemberPointerTraits<MemberPtr>::MemberType;

        PropertyInfo prop;
        prop.type_id = TypeId::Get<MemberType>();
        prop.name = name;
        prop.size = sizeof(MemberType);

        // 오프셋 계산
        prop.offset = reinterpret_cast<usize>(&(static_cast<T*>(nullptr)->*MemberPtr));

        // 메타데이터 설정
        prop.metadata.display_name = display_name.IsEmpty() ? name : display_name;
        prop.metadata.category = category;
        prop.metadata.tooltip = ""; // TODO: 나중에 metadata를 외부에서 받던가 해서 수정

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
