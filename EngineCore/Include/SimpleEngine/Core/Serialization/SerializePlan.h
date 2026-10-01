#pragma once

#include "SimpleEngine/Core/Container/ArrayView.h"
#include "SimpleEngine/Core/Container/StringView.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Core/Reflection/TypeId.h"
#include "SimpleEngine/Core/Reflection/TypeShape.h"
#include "SimpleEngine/Core/Reflection/ValueOps.h"
#include "SimpleEngine/Core/Serialization/Archive.h"
#include "SimpleEngine/Core/Serialization/SerializeOpsRegistry.h"

#include <variant>


namespace se
{
struct SerializePlan;

/** 평탄화된 구조체 필드 하나의 컴파일 결과 */
struct FieldStep
{
    StringView name;
    usize offset = 0;
    const SerializePlan* plan = nullptr;
};

/** 아직 컴파일 중인 Plan의 단계 */
struct PendingStep{};

/** 트레이트 또는 허용된 산술 타입의 leaf 노드 */
struct LeafStep
{
    SerializeOps ops{};
};

/** 베이스까지 평탄화된 필드 목록(선언 순서, 부모 필드가 먼저). */
struct StructSteps
{
    ArrayView<const FieldStep> fields;
};

/** Array-like 컨테이너의 원소 Plan과 ArrayOps */
struct ArraySteps
{
    const SerializePlan* element = nullptr;
    const ArrayOps* ops = nullptr;

    /** 원소를 메모리 바이트 그대로 쓰고 읽을 수 있으면(원소가 trivially packable이고 memcpy로 옮길 수 있음) 원소 하나의 바이트 수, 아니면 0 */
    usize raw_element_size = 0;
};

/** Set/Map을 읽을 때 임시 원소를 만들기 위한 정보 */
struct ElementInfo
{
    usize size = 0;
    usize alignment = 0;
    const ValueOps* value_ops = nullptr;
};

/** Set-like 컨테이너의 원소 Plan과 SetOps */
struct SetSteps
{
    const SerializePlan* element = nullptr;
    const SetOps* ops = nullptr;
    ElementInfo element_info;
};

/** Map-like 컨테이너의 key/value Plan과 MapOps */
struct MapSteps
{
    const SerializePlan* key = nullptr;
    const SerializePlan* value = nullptr;
    const MapOps* ops = nullptr;
    ElementInfo key_info;
    ElementInfo value_info;
};

/** Optional의 내부 값 Plan과 OptionalOps */
struct OptionalSteps
{
    const SerializePlan* inner = nullptr;
    const OptionalOps* ops = nullptr;
};

/** enum을 쓸 정수 폭과 부호, 이름 목록 */
struct EnumStep
{
    EIntWidth width = EIntWidth::Bits32;
    bool is_signed = false;
    ArrayView<const EnumEntry> entries;
};

/** 형태별 직렬화 단계 */
using PlanSteps = std::variant<PendingStep, LeafStep, StructSteps, ArraySteps, SetSteps, MapSteps, OptionalSteps, EnumStep>;

/**
 * 타입 하나를 직렬화하는 데 필요한 정보를 리플렉션 TypeInfo에서 뽑아 둔 결과
 * 타입마다 한 번만 컴파일되고(SerializePlanRegistry), 이후 직렬화할 때는 레지스트리 조회나 어노테이션 스캔 없이 바로 사용합니다.
 */
struct SE_CORE_API SerializePlan
{
    TypeId type;
    PlanSteps steps;

    /**
     * Packed 인코딩이 메모리 바이트와 똑같은 타입(trivially packable)이면 true입니다.
     * bool을 뺀 산술 타입으로 이루어진 필드이면서, 오프셋 순서로 패딩 없이 등록한 구조체가 해당합니다. enum과 트레이트 타입은 해당하지 않습니다.
     */
    bool is_trivially_packable = false;

    /** 이 Plan에 모든 타입의 서술로 스키마 해시를 계산합니다.
     */
    [[nodiscard]] u64 SchemaHash() const;
};
} // namespace se
