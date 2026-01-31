#include "Asset/EditorAssetSubsystem.h"

#include "SimpleEngine/Core/Serialization/TomlArchive.h"
#include "SimpleEngine/Core/Subsystem/SubsystemRegistration.h"
#include "SimpleEngine/Utility/FileSystem.h"
#include "SimpleEngine/Utility/StringUtils.h"
#include "SimpleEngine/Utility/SubsystemUtils.h"
#include "SimpleEngine/Utility/VFS.h"


namespace se::editor::asset
{
SE_REGISTER_SUBSYSTEM(EditorAssetSubsystem)
    .DependsOn<se::asset::AssetSubsystem>();

bool EditorAssetSubsystem::Initialize()
{
    asset_manager = &GetSubsystemChecked<se::asset::AssetSubsystem>().GetAssetManager_DEPRECATED();
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
    VFS::Get().VisitMounts([this](
        [[maybe_unused]] std::string_view scheme,
        const Path& physical_path,
        [[maybe_unused]] int32 priority
    ) {
        if (!physical_path.Exists())
        {
            return;
        }

        for (const auto& entry : FileSystem::ReadDir(physical_path))
        {
            // 폴더 건너뛰기
            if (entry.IsDirectory())
            {
                continue;
            }

            // .meta 파일은 건너뛰기
            const Path path = entry.GetPath();
            if (path.Extension().ValueOrDefault() == ".meta")
            {
                continue;
            }

            // Registry에 등록 및 .meta 생성
            ImportAsset(path);
        }
    });
}

void EditorAssetSubsystem::ImportAsset(const Path& physical_path)
{
    // .meta 처리
    if (auto entry_opt = ProcessMetaFile(physical_path))
    {
        // Registry에 등록
        asset_manager->GetRegistry().AddEntry(std::move(entry_opt).Value());
    }
}

// ReSharper disable once CppMemberFunctionMayBeConst
Optional<se::asset::AssetEntry_DEPRECATED> EditorAssetSubsystem::ProcessMetaFile(const Path& physical_path)
{
    // 지원하는 확장자인지 확인
    const Optional ext_opt = physical_path.Extension();
    if (!ext_opt.HasValue())
    {
        ConsoleLog(ELogLevel::Warning, "Cannot process meta file: file has no extension: {}", physical_path);
        return std::nullopt;
    }

    const StringName ext_name = ext_opt->CStr();
    Optional info_opt = asset_manager->GetExtensionInfo(ext_name);
    if (!info_opt.HasValue())
    {
        // 지원하지 않는 확장자는 조용히 무시
        return std::nullopt;
    }

    // 가상 경로 계산 (Physical -> Virtual)
    Optional vpath_opt = physical_path.ToVirtual();
    if (!vpath_opt.HasValue())
    {
        ConsoleLog(ELogLevel::Warning, "Cannot resolve virtual path for: {}", physical_path);
        return std::nullopt;
    }

    // .meta 파일 관련
    Path meta_path = physical_path;
    meta_path += ".meta";

    se::asset::AssetEntry_DEPRECATED entry;
    entry.import_settings = asset_manager->CreateDefaultSettingsForFile(physical_path);

    if (meta_path.Exists())
    {
        auto result = toml::parse_file(meta_path.ToString().CStr());
        if (!result)
        {
            ConsoleLog(ELogLevel::Error, "Failed to parse meta file: {}", meta_path);
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

        std::ofstream ofs{ meta_path.ToString().CStr() };
        if (ofs.is_open())
        {
            ofs << table;
            ofs.close();
        }
        else
        {
            ConsoleLog(ELogLevel::Error, "Failed to write meta file: {}", meta_path);
            return std::nullopt;
        }
    }

    return entry;
}
}  // namespace se::editor::asset
