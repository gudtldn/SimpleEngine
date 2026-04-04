#pragma once

#include "SimpleEditor/UI/IEditorPanel.h"


namespace se::editor
{
/**
 * World의 Resource를 표시하고 편집하는 패널
 */
class WorldResourcePanel : public IEditorPanel
{
public:
    [[nodiscard]] virtual const char* GetName() const override;

protected:
    virtual void DrawContent() override;
};
} // namespace se::editor
