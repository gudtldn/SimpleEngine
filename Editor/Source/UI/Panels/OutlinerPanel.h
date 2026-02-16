#pragma once
#include "SimpleEditor/UI/IEditorPanel.h"


namespace se::editor
{
class OutlinerPanel : public IEditorPanel
{
public:
    [[nodiscard]] virtual const char* GetName() const override;
    virtual void Draw() override;
};
}
