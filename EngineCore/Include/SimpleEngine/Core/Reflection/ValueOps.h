#pragma once

#include "SimpleEngine/Core/Container/Optional.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Utility/Overloaded.h"

#include <utility>
#include <variant>


namespace se
{
/** Array-like 컨테이너(Array, FixedArray)의 연산 */
struct ArrayOps
{
    /** 요소 개수 (Len) */
    usize (*len)(const void* container);

    /** 가장 첫 번째 요소의 읽기 전용 주소 (Data) */
    const void* (*data)(const void* container);

    /** 가장 첫 번째 요소의 수정 가능한 주소 (Data) */
    void* (*data_mut)(void* container);

    /** index번째 요소의 읽기 전용 주소 (operator[]) */
    const void* (*element_at)(const void* container, usize index);

    /** index번째 요소의 수정 가능한 주소 (operator[]) */
    void* (*element_at_mut)(void* container, usize index);

    /** 값 초기화하며 크기 변경 (Resize). FixedArray처럼 불가능하면 nullptr */
    void (*resize)(void* container, usize count);

    /** 미초기화로 크기 변경 (ResizeUninitialized). 요소가 trivial할 때만 사용할 수 있습니다. */
    void (*resize_uninitialized)(void* container, usize count);

    /** 요소를 통째로 memcpy해도 되는지 */
    bool element_trivially_copyable;

    /** 요소가 포인터 타입인지 */
    bool element_is_pointer;
};

/** Set-like 컨테이너(HashSet, Set, FlatSet)의 연산 */
struct SetOps
{
    /** 요소 개수 (Len) */
    usize (*len)(const void* container);

    /** 전부 비웁니다 (Clear) */
    void (*clear)(void* container);

    /** 미리 용량 확보 (Reserve). 지원하지 않으면 nullptr */
    void (*reserve)(void* container, usize count);

    /** element를 move해서 삽입합니다 (Emplace). 이동 생성 불가 타입이면 nullptr */
    void (*emplace_moved)(void* container, void* element);

    /**
     * 모든 요소를 순회합니다.
     * @note 요소는 해시 불변식 때문에 수정할 수 없으므로 컨테이너도 const로 받습니다.
     */
    void (*for_each)(const void* container, void (*visit)(const void* element, void* user_data), void* user_data);
};

/** Map-like 컨테이너(HashMap, Map, FlatMap)의 연산 */
struct MapOps
{
    /** 엔트리 개수 (Len) */
    usize (*len)(const void* container);

    /** 전부 비웁니다 (Clear) */
    void (*clear)(void* container);

    /** 미리 용량 확보 (Reserve). 지원하지 않으면 nullptr */
    void (*reserve)(void* container, usize count);

    /** key/value를 move해서 삽입합니다 (Emplace). 이동 생성 불가 타입이면 nullptr */
    void (*emplace_moved)(void* container, void* key, void* value);

    /** 모든 엔트리를 읽기 전용으로 순회합니다. */
    void (*for_each)(const void* container, void (*visit)(const void* key, const void* value, void* user_data), void* user_data);

    /** 모든 엔트리를 순회하며 value를 수정할 수 있게 합니다. key는 해시 불변식 때문에 수정할 수 없습니다. */
    void (*for_each_mut)(void* container, void (*visit)(const void* key, void* value, void* user_data), void* user_data);
};

/** Optional의 연산 */
struct OptionalOps
{
    /** 값을 갖고 있는지 (HasValue) */
    bool (*has_value)(const void* optional);

    /** 내부 값의 읽기 전용 주소 (Value) */
    const void* (*value)(const void* optional);

    /** 내부 값의 수정 가능한 주소 (Value) */
    void* (*value_mut)(void* optional);

    /** 내부 값을 기본 생성하고 그 주소를 돌려줍니다 (Emplace). Optional<T&>처럼 불가능하면 nullptr */
    void* (*emplace)(void* optional);

    /** 값을 비웁니다 (Reset) */
    void (*reset)(void* optional);
};

/** TypeShape와 짝을 이루는 형태별 연산. 컨테이너가 아니면 monostate입니다. */
using ShapeOps = std::variant<std::monostate, ArrayOps, SetOps, MapOps, OptionalOps>;

/**
 * 타입 하나에 대한 값 연산 모음.
 * @note 지원 불가능한 연산은 전부 nullptr입니다.
 */
struct ValueOps
{
    /** 주어진 저장 공간에 기본 생성합니다. 기본 생성 불가 타입이면 nullptr */
    void (*default_construct_at)(void* storage);

    /** 소멸시킵니다. 소멸 불가 타입이면 nullptr */
    void (*destruct_at)(void* object);

    /** 형태별 연산. 컨테이너가 아니면 monostate */
    ShapeOps shape_ops;

    /** Array-like가 아니면 NullOpt */
    [[nodiscard]] Optional<const ArrayOps&> AsArray() const { return VariantGet<ArrayOps>(shape_ops); }

    /** Set-like가 아니면 NullOpt */
    [[nodiscard]] Optional<const SetOps&> AsSet() const { return VariantGet<SetOps>(shape_ops); }

    /** Map-like가 아니면 NullOpt */
    [[nodiscard]] Optional<const MapOps&> AsMap() const { return VariantGet<MapOps>(shape_ops); }

    /** Optional이 아니면 NullOpt */
    [[nodiscard]] Optional<const OptionalOps&> AsOptional() const { return VariantGet<OptionalOps>(shape_ops); }

    /**
     * 형태별로 분기합니다.
     * @note monostate(컨테이너 아님)를 포함해 하나라도 빠뜨리면 컴파일 에러입니다.
     */
    template <typename... Fns>
    decltype(auto) VisitShape(Fns&&... fns) const
    {
        return std::visit(Overloaded<std::decay_t<Fns>...>{ std::forward<Fns>(fns)... }, shape_ops);
    }
};
} // namespace se
