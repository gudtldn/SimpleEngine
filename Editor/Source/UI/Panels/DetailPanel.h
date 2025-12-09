#pragma once
#include "UI/Panels/IEditorPanel.h"
#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Reflection/Meta.h"


namespace se::editor::ui
{
class DetailPanel : public IEditorPanel
{
public:
    DetailPanel();

    [[nodiscard]] virtual const char* GetName() const override;
    virtual void Draw() override;

private:
    Array<refl::TypeInfo> components;
};
}  // namespace se::editor::ui
