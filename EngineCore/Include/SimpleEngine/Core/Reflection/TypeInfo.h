#pragma once

#include "SimpleEngine/Core/Container/ArrayView.h"
#include "SimpleEngine/Core/Container/Optional.h"
#include "SimpleEngine/Core/Container/StringView.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Core/Reflection/TypeId.h"
#include "SimpleEngine/Core/Reflection/TypeShape.h"
#include "SimpleEngine/Utility/Overloaded.h"

#include <utility>
#include <variant>


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

    /** 내부 구조가 없는 타입인지 */
    [[nodiscard]] constexpr bool IsOpaque() const { return std::holds_alternative<OpaqueInfo>(shape); }

    /** 구조체/클래스가 아니면 NullOpt */
    [[nodiscard]] constexpr Optional<const StructInfo&> AsStruct() const { return ShapeAs<StructInfo>(); }

    /** Array-like가 아니면 NullOpt */
    [[nodiscard]] constexpr Optional<const ArrayInfo&> AsArray() const { return ShapeAs<ArrayInfo>(); }

    /** Set-like가 아니면 NullOpt */
    [[nodiscard]] constexpr Optional<const SetInfo&> AsSet() const { return ShapeAs<SetInfo>(); }

    /** Map-like가 아니면 NullOpt */
    [[nodiscard]] constexpr Optional<const MapInfo&> AsMap() const { return ShapeAs<MapInfo>(); }

    /** Optional이 아니면 NullOpt */
    [[nodiscard]] constexpr Optional<const OptionalInfo&> AsOptional() const { return ShapeAs<OptionalInfo>(); }

    /** enum이 아니면 NullOpt */
    [[nodiscard]] constexpr Optional<const EnumInfo&> AsEnum() const { return ShapeAs<EnumInfo>(); }

    /**
     * 모양별로 분기합니다.
     * @note 7가지 모양 중 하나라도 처리하지 않으면 컴파일 에러입니다. (전수 처리 보장)
     */
    template <typename... Fns>
    constexpr decltype(auto) VisitShape(Fns&&... fns) const
    {
        return std::visit(Overloaded<std::decay_t<Fns>...>{ std::forward<Fns>(fns)... }, shape);
    }

private:
    template <typename Info>
    [[nodiscard]] constexpr Optional<const Info&> ShapeAs() const
    {
        if (const Info* info = std::get_if<Info>(&shape))
        {
            return *info;
        }
        return NullOpt;
    }
};
} // namespace se
