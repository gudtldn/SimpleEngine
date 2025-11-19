#include "Asset/EditorAssetSubsystem.h"

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
    // 지원하는 확장자인지 확인
    if (!asset_manager->GetTypeFromExtension(physical_path.extension()).HasValue())
    {
        return;
    }

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
    std::filesystem::path meta_path = physical_path;
    meta_path += ".meta";

    se::asset::AssetEntry entry;
    if (std::filesystem::exists(meta_path))
    {
        // TODO: [Load] 기존 메타 파일 읽기
        // std::ifstream ... >> json
        // entry.guid = json["guid"];
        // entry.asset_type = json["type"];
    }
    else
    {
        // [New] 메타 파일 생성
        entry.guid = Guid::NewGuid();
        entry.asset_type = asset_manager->GetTypeFromExtension(physical_path.extension()).Value();

        // TODO: [Save] 메타 파일 쓰기
        // std::ofstream ... << json
    }

    // 가상 경로 계산 (Physical -> Virtual)
    Optional<VPath> vpath_opt = utility::PathResolver::Get().Unresolve(physical_path);
    if (!vpath_opt.HasValue())
    {
        return std::nullopt;
    }

    entry.virtual_path = vpath_opt.Value();
    return entry;
}
}
