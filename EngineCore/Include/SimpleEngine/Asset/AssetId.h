#pragma once

#include "SimpleEngine/Core/Serialization/Archive.h"
#include "SimpleEngine/Core/Types/Guid.h"


namespace se
{
/**
 * 에셋을 고유하게 식별하는 GUID 래퍼 클래스
 */
class SE_CORE_API AssetId
{
public:
    static const AssetId invalid;

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
    usize operator()(const se::AssetId& asset_id) const noexcept
    {
        return std::hash<se::Guid>{}(asset_id.GetGuid());
    }
};
