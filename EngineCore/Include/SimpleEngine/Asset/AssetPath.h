#pragma once

#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/Types/Path.h"
#include "SimpleEngine/Utility/HashUtils.h"


namespace se::asset
{
/**
 * @todo docs
 */
class SE_CORE_API AssetPath
{
public:
    AssetPath() = default;
    AssetPath(StringView full_path_str);
    AssetPath(const Path& in_file_path, StringView in_sub_asset_name);

public:
    [[nodiscard]] FORCE_INLINE const Path& GetFilePath() const { return file_path; }
    [[nodiscard]] FORCE_INLINE const String& GetSubAssetName() const { return sub_asset_name; }
    [[nodiscard]] FORCE_INLINE bool HasSubAsset() const { return !sub_asset_name.IsEmpty(); }

    [[nodiscard]] String ToString() const;

    [[nodiscard]] bool operator==(const AssetPath&) const = default;
    [[nodiscard]] auto operator<=>(const AssetPath&) const = default;

private:
    Path file_path;
    String sub_asset_name;
};
}  // namespace se::asset

template <>
struct std::hash<se::asset::AssetPath>
{
    size_t operator()(const se::asset::AssetPath& path) const noexcept
    {
        usize hash = 0;
        se::HashUtils::Combine(hash, path.GetFilePath(), path.GetSubAssetName());
        return hash;
    }
};
