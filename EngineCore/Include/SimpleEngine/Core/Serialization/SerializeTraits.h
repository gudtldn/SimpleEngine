#pragma once

#include "SimpleEngine/Core/Serialization/Archive.h"

#include <concepts>


namespace se
{
/**
 * 저장 표현이 메모리 표현과 다른 타입의 확장 지점입니다.
 *
 * 특수화 시 아래 상수와 두 static 함수를 정의해야 합니다:
 *   static constexpr u32 FORMAT_VERSION = 1;
 *   static void Write(ArchiveWriter& writer, const T& value);
 *   static void Read(ArchiveReader& reader, T& value);
 */
template <typename T>
struct SerializeTraits;

/** T가 SerializeTraits로 커스텀 직렬화를 갖는지 확인합니다. */
template <typename T>
concept HasSerializeTraits = requires (ArchiveWriter& writer, ArchiveReader& reader, const T& in, T& out)
{
    { SerializeTraits<T>::FORMAT_VERSION } -> std::convertible_to<u32>;
    SerializeTraits<T>::Write(writer, in);
    SerializeTraits<T>::Read(reader, out);
};
} // namespace se
