#include "SimpleEngine/Asset/AssetPath.h"

#include <utility>


namespace se::asset
{
AssetPath::AssetPath(StringView full_path_str)
{
    const Optional separator_pos_opt = full_path_str.FindLast('#');
    if (!separator_pos_opt)
    {
        file_path = full_path_str;
        return;
    }

    file_path = full_path_str.Substr(0, *separator_pos_opt);
    sub_asset_name = full_path_str.Substr(*separator_pos_opt + 1);
}

AssetPath::AssetPath(VPath in_file_path, StringView in_sub_asset_name)
    : file_path(std::move(in_file_path))
    , sub_asset_name(in_sub_asset_name)
{
}

String AssetPath::ToString() const
{
    if (HasSubAsset())
    {
        return String::Format("{}#{}", file_path, sub_asset_name);
    }
    return file_path.ToString();
}
}  // namespace se::asset
