#pragma once
#include "UI/Panels/IEditorPanel.h"


namespace se::editor::ui
{
class DetailPanel : public IEditorPanel
{
public:
    [[nodiscard]] virtual const char* GetName() const override;
    virtual void Draw() override;
};
}
