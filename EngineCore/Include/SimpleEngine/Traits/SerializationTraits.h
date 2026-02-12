#pragma once


namespace se
{
// Forward declarations
class ArchiveV2;
}  // namespace se

namespace se::traits
{
/** Serialize 할 수 있는 타입만 받습니다. */
template <typename T>
concept Serializable = requires(ArchiveV2& ar, T& value)
{
    // ADL Serialize 존재 여부 판별
    Serialize(ar, value);
};
} // namespace se::traits
