#pragma once

#include "SimpleEditor/UI/IEditorPanel.h"


namespace se::editor
{
class DebugPanel : public IEditorPanel
{
public:
    DebugPanel();

    [[nodiscard]] virtual const char* GetName() const override;

protected:
    [[nodiscard]] virtual ImGuiWindowFlags GetWindowFlags() const override;
    virtual void DrawContent() override;
};
} // namespace se::editor
