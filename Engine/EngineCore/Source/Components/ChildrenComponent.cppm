export module SE.Components:ChildrenComponent;

import SE.Core;
import SE.Types;


/**
 * 현재 Entity의 자식 Entity를 지정합니다.
 */
export struct ChildrenComponent
{
    se::vector<se::core::ecs::Entity> children;
};
