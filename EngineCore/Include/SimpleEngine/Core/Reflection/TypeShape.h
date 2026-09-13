#pragma once

#include "SimpleEngine/Core/Container/ArrayView.h"
#include "SimpleEngine/Core/Container/StringView.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Core/Reflection/TypeId.h"

#include <variant>


namespace se
{
/**
 * 어노테이션 태그 하나에 대한 타입 소거(type-erased) 참조
 * value는 static 수명의 태그 인스턴스를 가리킵니다.
 */
struct AnnotationRef
{
    /** Annotaion의 타입 식별자 */
    TypeId tag;

    /** Annotation이 가지고 있는 데이터 */
    const void* value = nullptr;
};

/** 열거형(enum)의 항목을 나타내는 Entry */
struct EnumEntry
{
    /** Enumerate의 값 */
    i64 value = 0;

    /** Enumerate의 이름 */
    StringView name;

    [[nodiscard]] constexpr auto operator<=>(const EnumEntry& other) const
    {
        return value <=> other.value;
    }
};

/** 멤버 변수의 리플렉션 정보 */
struct FieldInfo
{
    /** 멤버 변수의 이름 */
    StringView name;

    /** 멤버 변수의 타입 식별자 */
    TypeId type;

    /** 멤버 변수가 위치하고있는 메모리 오프셋 */
    usize offset = 0;

    /** 멤버 변수의 Annotation 정보 */
    ArrayView<const AnnotationRef> annotations;
};

/** 클래스/구조체의 부모 정보 */
struct BaseInfo
{
    /** 부모의 타입 식별자 */
    TypeId type;

    /** 부모가 시작하는 위치 */
    usize offset = 0;
};

/** 클래스/구조체의 정보 */
struct StructInfo
{
    /** 상속된 목록들 */
    ArrayView<const BaseInfo> bases;

    /** 구조체가 가지고있는 Field 정보 목록 */
    ArrayView<const FieldInfo> fields;
};

/** Array/FixedArray등 순서가 있는 컨테이너의 정보 */
struct ArrayInfo
{
    TypeId element;
};

/** HashSet등 중복 없는 컨테이너의 정보 */
struct SetInfo
{
    TypeId element;
};

/** HashMap등 Key-Value 컨테이너의 정보 */
struct MapInfo
{
    TypeId key;
    TypeId value;
};

/** Optional의 정보 */
struct OptionalInfo
{
    TypeId inner;
};

/** 열거형(enum)의 정보 */
struct EnumInfo
{
    TypeId underlying;
    ArrayView<const EnumEntry> entries;
};

/** 내부 구조가 없는 타입의 정보 (예: i32, String, Guid, TypeId) */
struct OpaqueInfo{};

/** 타입의 구조적 분류 */
using TypeShape = std::variant<OpaqueInfo, StructInfo, ArrayInfo, SetInfo, MapInfo, OptionalInfo, EnumInfo>;
} // namespace se
