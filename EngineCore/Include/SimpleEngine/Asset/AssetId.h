#pragma once
#include "SimpleEngine/Core/Types/Guid.h"


namespace se::asset
{
/**
 * @todo docs
 */
class AssetId
{
public:
    AssetId() = default;
    explicit AssetId(const Guid& in_guid) : guid(in_guid) {}

    [[nodiscard]] bool IsValid() const noexcept { return guid.IsValid(); }
    [[nodiscard]] const Guid& GetGuid() const noexcept { return guid; }

    [[nodiscard]] explicit operator bool() const noexcept { return IsValid(); }
    [[nodiscard]] bool operator==(const AssetId&) const noexcept = default;

private:
    Guid guid;
};
}  // namespace se::asset

template <>
struct std::hash<se::asset::AssetId>
{
    size_t operator()(const se::asset::AssetId& asset_id) const noexcept
    {
        return std::hash<se::Guid>{}(asset_id.GetGuid());
    }
};
