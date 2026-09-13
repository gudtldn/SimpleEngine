#pragma once

#include "SimpleEngine/Core/Container/ArrayView.h"
#include "SimpleEngine/Core/Container/StringView.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Core/Reflection/TypeId.h"
#include "SimpleEngine/Core/Reflection/TypeShape.h"


namespace se
{
/**
 * 타입 리플렉션 정보를 표현하는 구조체
 */
struct TypeInfo
{
    /** Type의 컴파일타임 타입 식별자 */
    TypeId id;

    /** Type의 이름 */
    StringView name;

    /** Type의 전체 메모리 크기 (sizeof) */
    usize size = 0;

    /** Type의 메모리 정렬 요구사항 (alignof) */
    usize alignment = 0;

    /* 타입의 종류 */
    TypeShape shape;

    /** Type의 어노테이션 목록 */
    ArrayView<const AnnotationRef> annotations;
};
} // namespace se
