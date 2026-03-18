#pragma once

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Graphics/Scene/DrawCommand.h"


namespace se::graphics
{
/**
 * 한 프레임에서 렌더링할 드로우 커맨드의 스냅샷
 */
struct SceneDrawData
{
    Array<DrawCommand> opaque_commands;
    // Array<DrawCommand> transparent_commands; // 추후 투명 오브젝트 분류 시 추가
};
} // namespace se::graphics
