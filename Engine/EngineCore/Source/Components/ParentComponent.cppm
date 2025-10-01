export module SE.Components:ParentComponent;

import SE.Core;
import SE.Types;


/**
 * 현재 Entity의 부모 Entity를 지정합니다.
 */
export struct ParentComponent
{
    se::core::ecs::Entity parent;
};
