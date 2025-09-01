export module SE.Components:MaterialHandleComponent;

import SE.Types;


/**
 * Entity가 사용할 재질(Material) 리소스의 ID를 지정하는 컴포넌트
 */
export struct MaterialHandleComponent
{
    uint32 material_id = 0;
};
