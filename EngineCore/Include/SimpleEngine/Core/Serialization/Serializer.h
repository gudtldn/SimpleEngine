#pragma once

#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/Error/Expected.h"
#include "SimpleEngine/Core/Serialization/Archive.h"
#include "SimpleEngine/Core/Serialization/SerializePlanRegistry.h"


namespace se
{
/** Serialize/Deserialize가 실패한 위치와 메시지를 담은 구조체 */
struct SerializeError
{
    /**
     * 실패 지점까지의 경로.
     * 필드는 ".name"(맨 앞은 점 없이), 시퀀스 원소는 "[i]", 맵 엔트리는 "[i].key" 또는 "[i].value"로 표기합니다. 예: "items[3].value"
     */
    String path;

    /** archive가 기록한 첫 오류 메시지 */
    String message;
};

namespace serde
{
/**
 * plan을 따라 value를 순회하며 writer에 씁니다.
 * @note writer를 스레드마다 따로 두면 여러 스레드에서 동시에 호출할 수 있습니다.
 */
[[nodiscard]] SE_CORE_API Expected<void, SerializeError> Serialize(ArchiveWriter& writer, const SerializePlan& plan, const void* value);

/**
 * plan을 따라 reader에서 읽은 값을 value에 채웁니다.
 *
 * value는 유효한 객체이고 기본 생성된 상태라고 전제합니다. 데이터에 없는 struct 필드는 value의 현재 값을
 * 유지하므로(새 객체라면 기본값), 다시 로드할 때는 살아 있는 객체에 덮어 로드하지 않고 새 객체에 로드한 뒤 교체합니다.
 * 컨테이너(Array, Set, Map, Optional)는 기존 내용에 누적하지 않고 데이터로 교체합니다.
 * 실패해도 value는 파괴하거나 다시 대입할 수 있는 유효한 객체로 남지만, 내용은 보장하지 않습니다.
 *
 * @note reader를 스레드마다 따로 두면 여러 스레드에서 동시에 호출할 수 있습니다. 정적 초기화 중에는 호출하지 않습니다.
 */
[[nodiscard]] SE_CORE_API Expected<void, SerializeError> Deserialize(ArchiveReader& reader, const SerializePlan& plan, void* value);

/** T의 SerializePlan을 SerializePlanOf<T>()로 가져와 value를 writer에 씁니다. */
template <typename T>
[[nodiscard]] Expected<void, SerializeError> Serialize(ArchiveWriter& writer, const T& value)
{
    return Serialize(writer, SerializePlanOf<T>(), &value);
}

/** T의 SerializePlan을 SerializePlanOf<T>()로 가져와 reader에서 읽은 값을 value에 채웁니다. */
template <typename T>
[[nodiscard]] Expected<void, SerializeError> Deserialize(ArchiveReader& reader, T& value)
{
    return Deserialize(reader, SerializePlanOf<T>(), &value);
}
} // namespace serde
} // namespace se
