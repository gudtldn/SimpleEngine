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
} // namespace se::traits
