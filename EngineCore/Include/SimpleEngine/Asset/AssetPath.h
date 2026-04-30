#pragma once

#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/Serialization/Archive.h"
#include "SimpleEngine/Core/Types/VPath.h"
#include "SimpleEngine/Utility/HashUtils.h"


namespace se
{
/**
 * Asset의 소스 파일 경로와 Sub-Asset 이름을 결합한 식별 경로
 *
 * "Assets://Characters/Hero.fbx#Mesh_Body" 형태로 표현되며,
 * file_path("Assets://Characters/Hero.fbx")와 sub_asset_name("Mesh_Body")으로 구성됩니다.
 */
class SE_CORE_API AssetPath
{
public:
    AssetPath() = default;
    AssetPath(StringView full_path_str);
    AssetPath(VPath in_file_path, StringView in_sub_asset_name);

public:
    [[nodiscard]] FORCE_INLINE const VPath& GetFilePath() const { return file_path; }
    [[nodiscard]] FORCE_INLINE const String& GetSubAssetName() const { return sub_asset_name; }
    [[nodiscard]] FORCE_INLINE bool HasSubAsset() const { return !sub_asset_name.IsEmpty(); }

    [[nodiscard]] String ToString() const;

    [[nodiscard]] bool operator==(const AssetPath&) const = default;
    [[nodiscard]] auto operator<=>(const AssetPath&) const = default;

private:
    friend void Serialize(Archive& ar, AssetPath& path)
    {
        String file_str;
        if (ar.IsSaving())
        {
            file_str = path.file_path.ToString();
        }

        ar("file_path") << file_str;
        ar("sub_asset_name") << path.sub_asset_name;

        if (ar.IsLoading())
        {
            path.file_path = file_str;
        }
    }

    VPath file_path;
    String sub_asset_name;
};
} // namespace se

template <>
struct std::hash<se::AssetPath>
{
    size_t operator()(const se::AssetPath& path) const noexcept
    {
        usize hash = 0;
        se::HashUtils::Combine(hash, path.GetFilePath(), path.GetSubAssetName());
        return hash;
    }
};
