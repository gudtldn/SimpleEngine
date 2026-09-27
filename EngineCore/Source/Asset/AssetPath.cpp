#include "SimpleEngine/Asset/AssetPath.h"

#include "SimpleEngine/Core/Reflection/ReflectMacros.h"

#include <utility>


namespace se
{
AssetPath::AssetPath(StringView full_path_str)
{
    const auto separator_pos_opt = full_path_str.FindLast('#');
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
} // namespace se


// 멤버가 private라 AssetPath가 Registrar<AssetPath>를 friend로 둠. 필드 순서는 레거시 Serialize 훅과 같음
SE_REFLECT_BEGIN(se::AssetPath)
    SE_FIELD(file_path)
    SE_FIELD(sub_asset_name)
SE_REFLECT_END()
