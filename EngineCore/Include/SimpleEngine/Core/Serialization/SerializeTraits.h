#pragma once

#include "SimpleEngine/Core/Serialization/Archive.h"


namespace se
{
/**
 * 저장 표현이 메모리 표현과 다른 타입의 확장 지점입니다.
 *
 * 특수화 시 아래 두 static 함수를 정의해야 합니다:
 *   static void Write(Archive& ar, const T& value);
 *   static void Read(Archive& ar, T& value);
 */
template <typename T>
struct SerializeTraits;

/** T가 SerializeTraits로 커스텀 직렬화를 갖는지 확인합니다. */
template <typename T>
concept HasSerializeTraits = requires(Archive& ar, T& value, const T& const_value)
{
    SerializeTraits<T>::Write(ar, const_value);
    SerializeTraits<T>::Read(ar, value);
};
} // namespace se
