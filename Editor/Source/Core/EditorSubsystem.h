#pragma once
#include "Core/EditorSelection.h"
#include "SimpleEngine/Core/Subsystem//ISubsystem.h"


namespace se::editor
{
class EditorSubsystem : public ISubsystem
{
public:
    [[nodiscard]] virtual bool Initialize() override;
    virtual void Release() override;

public:
    [[nodiscard]] EditorSelection& GetSelection() { return selection; }
    [[nodiscard]] const EditorSelection& GetSelection() const { return selection; }

private:
    EditorSelection selection;
};
}
