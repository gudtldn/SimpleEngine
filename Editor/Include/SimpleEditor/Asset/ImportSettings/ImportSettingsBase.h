#pragma once

#include "SimpleEditor/EditorCommon.h"

#include "SimpleEngine/Core/Reflection/Registrar.h"
#include "SimpleEngine/Core/Reflection/Rtti.h"


namespace se::editor
{
/**
 * 에셋 임포트 설정(Import Settings)의 기본 클래스
 */
class SE_EDITOR_API ImportSettingsBase
{
public:
    SE_RTTI_ROOT()

    virtual ~ImportSettingsBase() = default;
};
} // namespace se::editor

SE_DECLARE_REFLECTION(se::editor::ImportSettingsBase, SE_EDITOR_API)
