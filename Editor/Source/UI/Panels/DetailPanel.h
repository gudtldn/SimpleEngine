#pragma once
#include "UI/Panels/IEditorPanel.h"
#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Reflection/Meta.h"


namespace se::editor
{
class DetailPanel : public IEditorPanel
{
public:
    DetailPanel();

    [[nodiscard]] virtual const char* GetName() const override;
    virtual void Draw() override;

private:
    Array<TypeInfo> components;
};
}  // namespace se::editor
