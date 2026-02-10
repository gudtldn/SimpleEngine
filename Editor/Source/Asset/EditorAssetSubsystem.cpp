#include "Asset/EditorAssetSubsystem.h"
#include "SimpleEngine/Core/Subsystem/SubsystemRegistration.h"


namespace se::editor
{
SE_REGISTER_SUBSYSTEM(EditorAssetSubsystem)
    .DependsOn<se::asset::AssetSubsystem>();

SE_BEGIN_REFLECT(EditorAssetSubsystem)
SE_END_REFLECT(EditorAssetSubsystem)

bool EditorAssetSubsystem::Initialize()
{
    // TODO: 캐시 불러오는 로직
    return true;
}

void EditorAssetSubsystem::Release()
{
    // TODO: 캐시 저장 로직
}
}  // namespace se::editor
