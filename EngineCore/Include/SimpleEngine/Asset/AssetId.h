#pragma once
#include "SimpleEngine/Core/Serialization/Archive.h"
#include "SimpleEngine/Core/Types/Guid.h"


namespace se
{
/**
 * @todo docs
 */
class SE_CORE_API AssetId
{
public:
    static const AssetId Invalid;

public:
    AssetId() = default;
    constexpr explicit AssetId(const Guid& in_guid) : guid(in_guid) {}

    [[nodiscard]] bool IsValid() const noexcept { return guid.IsValid(); }
    [[nodiscard]] const Guid& GetGuid() const noexcept { return guid; }

    [[nodiscard]] explicit operator bool() const noexcept { return IsValid(); }
    [[nodiscard]] bool operator==(const AssetId&) const noexcept = default;

private:
    friend void Serialize(Archive& ar, AssetId& asset_id)
    {
        ar("guid") << asset_id.guid;
    }

    Guid guid;
};
} // namespace se

template <>
struct std::hash<se::AssetId>
{
    size_t operator()(const se::AssetId& asset_id) const noexcept
    {
        return std::hash<se::Guid>{}(asset_id.GetGuid());
    }
};
