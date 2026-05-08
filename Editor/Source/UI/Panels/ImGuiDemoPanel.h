#pragma once
#include "SimpleEditor/UI/IEditorPanel.h"


namespace se::editor
{
/**
 * ImGui demo를 보여주는 패널
 */
class ImGuiDemoPanel final : public IEditorPanel
{
public:
    ImGuiDemoPanel();

    //~ IEditorPanel
    [[nodiscard]] virtual const char* GetName() const override;
    virtual void Draw() override;
    //~ IEditorPanel
};
} // namespace se::editor
