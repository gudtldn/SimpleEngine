#pragma once

#include "SimpleEditor/UI/IEditorPanel.h"

#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Math/Math.h"
#include "SimpleEngine/ECS/Entity.h"


namespace se::editor
{
class DetailPanel : public IEditorPanel
{
public:
    [[nodiscard]] virtual const char* GetName() const override;
    virtual void Draw() override;

private:
    struct CachedRotator
    {
        Quaternion source_quat = Quaternion::Identity();
        Rotator euler = Rotator::ZeroRotator();
        bool is_editing = false;
    };

    /** 선택된 컴포넌트 */
    TypeId selected_component_id;

    /** 선택 유효성 체크를 위한 마지막으로 선택된 Entity */
    Entity last_selected_entity;

    /** TransformComponent Rotation 편집용 Euler 캐시 */
    HashMap<Entity, CachedRotator> rotator_cache;
};
} // namespace se::editor
