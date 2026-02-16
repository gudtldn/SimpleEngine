#pragma once
#include "SimpleEditor/UI/IEditorPanel.h"


namespace se::editor
{
class ImGuiDemoPanel final : public IEditorPanel
{
public:
    ImGuiDemoPanel();

    //~ IEditorPanel
    [[nodiscard]] virtual const char* GetName() const override;
    virtual void Draw() override;
    //~ IEditorPanel
};
}  // namespace se::editor
