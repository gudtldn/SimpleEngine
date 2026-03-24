#pragma once

#include "SimpleEditor/UI/IEditorPanel.h"
#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Reflection/Meta.h"
#include "SimpleEngine/ECS/Entity.h"


namespace se::editor
{
class DetailPanel : public IEditorPanel
{
public:
    DetailPanel();

    [[nodiscard]] virtual const char* GetName() const override;
    virtual void Draw() override;

private:
    /** 캐싱된 컴포넌트 목록 */
    Array<TypeInfo> components;

    /** 선택된 컴포넌트 */
    TypeId selected_component_id;

    /** 선택 유효성 체크를 위한 마지막으로 선택된 Entity */
    Entity last_selected_entity;
};
} // namespace se::editor
