#pragma once
#include "UI/Panels/IEditorPanel.h"


namespace se::editor::ui
{
class ImGuiDemoPanel final : public IEditorPanel
{
public:
    //~ IEditorPanel
    [[nodiscard]] virtual const char* GetName() const override;
    virtual void Draw(EditorUIContext& context) override;
    //~ IEditorPanel
};
}
