#pragma once
#include <concepts>


namespace se
{
// forward declaration
class TypeId;
struct TypeInfo;

/**
 * 비침투형(Non-intrusive) 리플렉션 타입을 컴파일 타임에 마킹하기 위한 traits 구조체
 * SE_DECLARE_REFLECTION 매크로를 통해 특수화됩니다
 */
template <typename T>
struct ReflectionTraits
{
    static constexpr bool Enabled = false;
};

/**
 * SE_CLASS 매크로를 통해 침투형(Intrusive) 리플렉션이 구현된 타입인지 확인하는 Concept.
 * StaticTypeInfo(), GetTypeInfo(), GetTypeId() 멤버를 요구합니다.
 */
template <typename T>
concept IntrusiveReflectable = requires(const T& obj)
{
    { obj.StaticTypeInfo() } -> std::same_as<const TypeInfo&>;
    { obj.GetTypeInfo() } -> std::same_as<const TypeInfo&>;
    { obj.GetTypeId() } -> std::same_as<TypeId>;
};

/**
 * SE_DECLARE_REFLECTION 매크로를 통해 비침투형(Non-intrusive) 리플렉션이 마킹된 타입인지 확인하는 Concept.
 */
template <typename T>
concept NonIntrusiveReflectable = ReflectionTraits<T>::Enabled;

/**
 * 엔진 리플렉션 시스템에 등록된 타입인지 확인합니다.
 * 침투형(SE_CLASS) 또는 비침투형(SE_DECLARE_REFLECTION) 어느 쪽이든 해당되면 true
 */
template <typename T>
concept Reflectable = IntrusiveReflectable<T> || NonIntrusiveReflectable<T>;
} // namespace se


/**
 * 비침투형(Non-intrusive) 리플렉션 타입을 등록합니다.
 * @param type 등록할 클래스/구조체 이름
 */
#define SE_DECLARE_REFLECTION(type) \
template<> struct ::se::ReflectionTraits<type> \
{ \
    static constexpr bool Enabled = true; \
};
