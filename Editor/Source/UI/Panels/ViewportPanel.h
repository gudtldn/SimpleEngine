#pragma once

#include "SimpleEditor/UI/IEditorPanel.h"
#include "SimpleEngine/Core/Types/StringName.h"


namespace se::editor
{
/**
 * @todo docs
 */
class ViewportPanel : public IEditorPanel
{
public:
    explicit ViewportPanel(const StringName& in_viewport_id);

    [[nodiscard]] virtual const char* GetName() const override;
    virtual void Draw() override;

private:
    StringName viewport_id;
};
} // namespace se::editor
