#include "Asset/EditorAssetSubsystem.h"

#include "SimpleEngine/Core/Serialization/TomlArchive.h"
#include "SimpleEngine/Utility/StringUtils.h"

namespace fs = std::filesystem;


namespace se::editor::asset
{
bool EditorAssetSubsystem::Initialize()
{
    asset_manager = std::addressof(GetSubsystem<AssetSubsystem>()->GetAssetManager());
    SE_ASSERT(asset_manager != nullptr);

    // TODO: 캐시 불러오는 로직

    RefreshRegistry();
    return true;
}

void EditorAssetSubsystem::Release()
{
    // TODO: 캐시 저장 로직
}

void EditorAssetSubsystem::RefreshRegistry()
{
    utility::PathResolver::Get().VisitMountPoints([this](
        [[maybe_unused]] const StringName& scheme,
        const fs::path& physical_path,
        [[maybe_unused]] int32 priority
    ) {
        if (!std::filesystem::exists(physical_path))
        {
            return;
        }

        std::error_code ec;
        for (const auto& entry : fs::directory_iterator(physical_path, ec))
        {
            // 폴더 건너뛰기
            if (ec || entry.is_directory())
            {
                continue;
            }

            // .meta 파일은 건너뛰기
            const auto& path = entry.path();
            if (path.extension() == ".meta")
            {
                continue;
            }

            // Registry에 등록 및 .meta 생성
            ImportAsset(path);
        }
    });
}

void EditorAssetSubsystem::ImportAsset(const std::filesystem::path& physical_path)
{
    // .meta 처리
    if (auto entry_opt = ProcessMetaFile(physical_path))
    {
        // Registry에 등록
        asset_manager->GetRegistry().AddEntry(std::move(entry_opt).Value());
    }
}

// ReSharper disable once CppMemberFunctionMayBeConst
Optional<se::asset::AssetEntry> EditorAssetSubsystem::ProcessMetaFile(const std::filesystem::path& physical_path)
{
    // 지원하는 확장자인지 확인
    Optional type_opt = asset_manager->GetTypeFromExtension(physical_path.extension());
    if (!type_opt.HasValue())
    {
        return std::nullopt;
    }

    // 가상 경로 계산 (Physical -> Virtual)
    Optional vpath_opt = utility::PathResolver::Get().Unresolve(physical_path);
    if (!vpath_opt.HasValue())
    {
        return std::nullopt;
    }

    se::asset::AssetEntry entry;

    // TODO: 매번 std::shared_ptr를 만들어서 반환하는거 개선 및 .meta에 저장할 수 있도록
    entry.import_settings = asset_manager->GetSettingsForType(entry.asset_type);

    // .meta 파일 관련
    std::filesystem::path meta_path = physical_path;
    meta_path += ".meta";

    if (std::filesystem::exists(meta_path))
    {
        if (auto result = toml::parse_file(meta_path.u8string()))
        {
            core::TomlReader reader{ result.table() };
            reader << entry;
        }
        else
        {
            return std::nullopt;
        }
    }
    else
    {
        // Entry 정보 추가
        entry.guid = Guid::NewGuid();
        entry.asset_type = *type_opt;
        entry.virtual_path = std::move(vpath_opt).Value();

        toml::table table;
        core::TomlWriter writer{ table };

        writer << entry;

        std::ofstream ofs{ meta_path };
        ofs << table;
        ofs.close();
    }
    return entry;
}
}
