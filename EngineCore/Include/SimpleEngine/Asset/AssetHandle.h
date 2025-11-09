#pragma once
#include <utility>

#include "SimpleEngine/Core/Types/Guid.h"


namespace se::asset
{
/**
 * 특정 타입의 Asset에 대한 핸들
 * @tparam T 핸들이 참조하는 Asset의 타입 (ex: Texture, Material, Mesh)
 */
template <typename T>
class AssetHandle
{
public:
    using AssetType = T;

public:
    AssetHandle() = default;
    explicit AssetHandle(Guid in_guid);

public:
    [[nodiscard]] bool IsValid() const noexcept;
    [[nodiscard]] const Guid& GetGuid() const noexcept { return guid; }

public:
    [[nodiscard]] explicit operator bool() const noexcept { return IsValid(); }
    [[nodiscard]] bool operator==(const AssetHandle&) const noexcept = default;

private:
    Guid guid;
};

template <typename T>
AssetHandle<T>::AssetHandle(Guid in_guid)
    : guid(std::move(in_guid))
{
}

template <typename T>
bool AssetHandle<T>::IsValid() const noexcept
{
    return guid.IsValid();
}
}

template <typename T>
struct std::hash<se::asset::AssetHandle<T>>
{
    size_t operator()(const se::asset::AssetHandle<T>& handle) const noexcept
    {
        return std::hash<Guid>{}(handle.GetGuid());
    }
};
