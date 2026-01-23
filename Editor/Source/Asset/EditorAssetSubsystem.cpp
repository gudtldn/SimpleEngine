#include "Asset/EditorAssetSubsystem.h"

#include "SimpleEngine/Core/Serialization/TomlArchive.h"
#include "SimpleEngine/Core/Subsystem/SubsystemRegistration.h"
#include "SimpleEngine/Utility/StringUtils.h"
#include "SimpleEngine/Utility/SubsystemUtils.h"

namespace fs = std::filesystem;


namespace se::editor::asset
{
SE_REGISTER_SUBSYSTEM(EditorAssetSubsystem)
    .DependsOn<se::asset::AssetSubsystem>();

bool EditorAssetSubsystem::Initialize()
{
    asset_manager = &GetSubsystemChecked<se::asset::AssetSubsystem>().GetAssetManager();
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
Optional<se::asset::AssetEntry_DEPRECATED> EditorAssetSubsystem::ProcessMetaFile(const std::filesystem::path& physical_path)
{
    // 지원하는 확장자인지 확인
    const StringName ext_name = utility::ToString(physical_path.extension().c_str());
    Optional info_opt = asset_manager->GetExtensionInfo(ext_name);
    if (!info_opt.HasValue())
    {
        return std::nullopt;
    }

    // 가상 경로 계산 (Physical -> Virtual)
    Optional vpath_opt = utility::PathResolver::Get().Unresolve(physical_path);
    if (!vpath_opt.HasValue())
    {
        return std::nullopt;
    }

    // .meta 파일 관련
    std::filesystem::path meta_path = physical_path;
    meta_path += ".meta";

    se::asset::AssetEntry_DEPRECATED entry;
    entry.import_settings = asset_manager->CreateDefaultSettingsForFile(physical_path);

    if (std::filesystem::exists(meta_path))
    {
        auto result = toml::parse_file(meta_path.u8string());
        if (!result)
        {
            ConsoleLog(ELogLevel::Error, "Failed to parse meta file: {}", meta_path.string());
            return std::nullopt;
        }

        core::TomlReader reader{ result.table() };
        reader << entry;
    }
    else
    {
        // Entry 정보 추가
        entry.guid = Guid::NewGuid();
        entry.asset_type = info_opt->asset_type;
        entry.loader_type = info_opt->loader_type;
        entry.virtual_path = std::move(vpath_opt).Value();

        toml::table table;
        core::TomlWriter writer{ table };
        writer << entry;

        std::ofstream ofs{ meta_path };
        if (ofs.is_open())
        {
            ofs << table;
            ofs.close();
        }
        else
        {
            ConsoleLog(ELogLevel::Error, "Failed to write meta file: {}", meta_path.string());
            return std::nullopt;
        }
    }

    return entry;
}
}
