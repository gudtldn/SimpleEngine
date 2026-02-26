#pragma once

#include "SimpleEngine/Asset/AssetSubsystem.h"
#include "SimpleEngine/Core/Subsystem/SubsystemBase.h"
#include "SimpleEngine/Core/Types/Path.h"


namespace se::editor
{
// forward declaration
class AssetImporter;

/**
 * @todo docs
 */
class SE_ANNOTATION(=meta::Internal) EditorAssetSubsystem : public SubsystemBase
{
    SE_CLASS(EditorAssetSubsystem, SubsystemBase)

public:
    EditorAssetSubsystem();
    virtual ~EditorAssetSubsystem() override;

public:
    [[nodiscard]] virtual bool Initialize() override;
    virtual void Release() override;

private:
    /** Asset을 불러와 Registry 등록과 DDC로 저장합니다.  */
    bool CookAsset(const Path& file_path);

private:
    std::unique_ptr<AssetImporter> importer;
    asset::AssetSubsystem* asset_subsystem = nullptr;
};
}  // namespace se::editor
