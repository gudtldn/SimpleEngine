#pragma once

#include "SimpleEditor/EditorCommon.h"

#include "SimpleEditor/Core/EditorSelection.h"
#include "SimpleEngine/Core/Subsystem/SubsystemBase.h"


namespace se::editor
{
class SE_EDITOR_API SE_ANNOTATION(=meta::Internal) EditorSubsystem : public SubsystemBase
{
    SE_CLASS(EditorSubsystem, SubsystemBase)

public:
    [[nodiscard]] virtual bool Initialize() override;
    virtual void Release() override;

public:
    [[nodiscard]] EditorSelection& GetSelection() { return selection; }
    [[nodiscard]] const EditorSelection& GetSelection() const { return selection; }

private:
    EditorSelection selection;
};
} // namespace se::editor
