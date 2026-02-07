#include "Asset/Pipeline/ImportResult.h"


namespace se::asset
{
ImportError::ImportError(ECode code, String message, Path file_path)
    : code(code)
    , message(std::move(message))
    , file_path(std::move(file_path))
{
}

const char* ImportError::What() const noexcept
{
    return message.CStr();
}

const IError* ImportError::Source() const noexcept
{
    return source_error.get();
}

ImportError::ECode ImportError::GetCode() const noexcept
{
    return code;
}

const Path& ImportError::GetFilePath() const noexcept
{
    return file_path;
}

void ImportError::SetSource(std::unique_ptr<IError> source)
{
    source_error = std::move(source);
}


std::shared_ptr<IAsset> ImportResult::GetMainAsset() const
{
    if (main_asset_index < assets.Len())
    {
        return assets[main_asset_index];
    }
    return nullptr;
}

std::shared_ptr<IAsset> ImportResult::FindByName(StringView name) const
{
    if (const auto index_opt = name_to_index.Find(name))
    {
        if (*index_opt < assets.Len())
        {
            return assets[*index_opt];
        }
    }
    return nullptr;
}

std::shared_ptr<IAsset> ImportResult::GetAsset(uint32 index) const
{
    if (index < assets.Len())
    {
        return assets[index];
    }
    return nullptr;
}

Array<StringView> ImportResult::GetAllNames() const
{
    Array<StringView> names; // TODO: HashMap API 바꾸면 name_to_index.Keys<StringView>()로 수정
    names.Reserve(name_to_index.Len());

    for (const String& name : name_to_index | std::views::keys)
    {
        names.Push(name);
    }
    return names;
}
}  // namespace se::asset
