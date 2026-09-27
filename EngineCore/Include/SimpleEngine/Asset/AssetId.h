#pragma once

#include "SimpleEngine/Core/Reflection/Registrar.h"
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

// guid가 private라 SE_FIELD로 서술할 수 없으므로 Opaque로 등록하고, 직렬화는 AssetId.cpp의 SerializeTraits가 맡음
SE_REFLECT_OPAQUE(se::AssetId)
