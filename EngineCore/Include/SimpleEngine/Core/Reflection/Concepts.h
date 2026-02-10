#pragma once
#include <concepts>

#include "SimpleEngine/Core/Reflection/Meta.h"


namespace se
{
/**
 * 엔진 리플렉션 시스템에 등록된 타입인지 확인하는 Concept
 * SE_CLASS 매크로를 통해 GetTypeId()가 구현된 타입만 사용 가능합니다.
 */
template <typename T>
concept Reflectable = requires(const T& obj)
{
    { obj.StaticTypeInfo() } -> std::same_as<const TypeInfo&>;
    { obj.GetTypeInfo() } -> std::same_as<const TypeInfo&>;
    { obj.GetTypeId() } -> std::same_as<TypeId>;
};
} // namespace se
