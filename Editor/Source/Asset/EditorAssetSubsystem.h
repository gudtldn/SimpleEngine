#pragma once

#include "SimpleEngine/Asset/AssetSubsystem.h"
#include "SimpleEngine/Core/Subsystem/SubsystemBase.h"
#include "SimpleEngine/Core/Types/Path.h"


namespace se::editor
{
class SE_ANNOTATION(=meta::Internal) EditorAssetSubsystem : public SubsystemBase
{
    SE_CLASS(EditorAssetSubsystem, SubsystemBase)

public:
    [[nodiscard]] virtual bool Initialize() override;
    virtual void Release() override;
};
}  // namespace se::editor
