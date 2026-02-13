#pragma once
#include "SimpleEngine/Core/Reflection/Meta.h"


namespace se
{
// forward declaration
class TypeRegistry;

/**
 * 리플렉션 PropertyInfo 기반 자동 직렬화/역직렬화 함수.
 * 등록된 프로퍼티들을 순회하며 각 프로퍼티의 serialize 콜백을 호출합니다.
 *
 * - 기본적으로 모든 프로퍼티가 직렬화 대상입니다.
 * - EPropertyFlags::Transient가 설정된 프로퍼티는 건너뜁니다.
 * - 부모 타입이 존재하면 부모의 프로퍼티를 먼저 직렬화합니다. (재귀)
 *
 * @param ar 직렬화에 사용할 Archive
 * @param info 대상 타입의 TypeInfo
 * @param instance 직렬화할 객체의 포인터
 */
SE_CORE_API void AutoSerialize(Archive& ar, const TypeInfo& info, void* instance);
SE_CORE_API void AutoSerialize(Archive& ar, const TypeId& type_id, void* instance);
}  // namespace se
