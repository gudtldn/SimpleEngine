#pragma once


namespace se
{
// Forward declarations
class Archive;
}  // namespace se

namespace se::traits
{
/** Serialize 할 수 있는 타입만 받습니다. */
template <typename T>
concept Serializable = requires(Archive& ar, T& value)
{
    // ADL Serialize 존재 여부 판별
    Serialize(ar, value);
};

/**
 * 인라인(스칼라) 직렬화 가능한 타입만 받습니다.
 * Serializable과 달리 BeginObject/EndObject 없이 직접 값으로 직렬화됩니다.
 * TOML에서 테이블이 아닌 단일 값(문자열, 숫자 등)으로 저장될 타입에 사용합니다.
 *
 * 제약: Serialize(Archive&, T&) 와 SerializeInline(Archive&, T&)을 동시에 구현하면
 *       컴파일 에러가 발생합니다. (Archive::operator<< 내 static_assert)
 */
template <typename T>
concept InlineSerializable = requires(Archive& ar, T& value)
{
    SerializeInline(ar, value);
};
} // namespace se::traits
