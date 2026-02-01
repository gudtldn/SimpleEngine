#pragma once

#include "SimpleEngine/Asset/AssetSubsystem.h"
#include "SimpleEngine/Core/Subsystem/ISubsystem.h"
#include "SimpleEngine/Core/Types/Path.h"


namespace se::editor
{
class EditorAssetSubsystem : public ISubsystem
{
public:
    [[nodiscard]] virtual bool Initialize() override;
    virtual void Release() override;
};
}  // namespace se::editor
