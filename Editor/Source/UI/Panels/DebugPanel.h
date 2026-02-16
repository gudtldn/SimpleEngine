#pragma once

#include "SimpleEditor/UI/IEditorPanel.h"


namespace se::editor
{
class DebugPanel : public IEditorPanel
{
public:
    DebugPanel();

    [[nodiscard]] virtual const char* GetName() const override;
    virtual void Draw() override;
};
}  // namespace se::editor
