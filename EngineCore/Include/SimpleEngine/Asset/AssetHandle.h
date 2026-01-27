#pragma once
#include <concepts>
#include <utility>

#include "SimpleEngine/Asset/AssetId.h"
#include "SimpleEngine/Asset/Types/IAsset.h"


namespace se::asset
{
/**
 * 특정 타입의 Asset에 대한 핸들
 * @tparam T 핸들이 참조하는 Asset의 타입 (ex: Texture, Material, Mesh)
 */
template <typename T>
    requires std::derived_from<T, IAsset>
class AssetHandle
{
public:
    using AssetType = T;

public:
    AssetHandle() = default;
    explicit AssetHandle(const Guid& in_guid);

public:
    [[nodiscard]] bool IsValid() const noexcept;
    [[nodiscard]] const Guid& GetGuid() const noexcept { return id.GetGuid(); }

public:
    // ReSharper disable once CppNonExplicitConversionOperator
    [[nodiscard]] operator AssetId() const noexcept { return id; }

    [[nodiscard]] explicit operator bool() const noexcept { return IsValid(); }
    [[nodiscard]] bool operator==(const AssetHandle&) const noexcept = default;

private:
    AssetId id;
};

template <typename T>
    requires std::derived_from<T, IAsset>
AssetHandle<T>::AssetHandle(const Guid& in_guid)
    : id(in_guid)
{
}

template <typename T>
    requires std::derived_from<T, IAsset>
bool AssetHandle<T>::IsValid() const noexcept
{
    return id.IsValid();
}
}  // namespace se::asset

template <typename T>
    requires std::derived_from<T, se::asset::IAsset>
struct std::hash<se::asset::AssetHandle<T>> // NOLINT(*-dcl58-cpp)
{
    size_t operator()(const se::asset::AssetHandle<T>& handle) const noexcept
    {
        return std::hash<se::Guid>{}(handle.GetGuid());
    }
};
