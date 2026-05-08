#pragma once

#include "SimpleEditor/EditorCommon.h"

#include "SimpleEngine/Core/Reflection/Reflect.h"


namespace se::editor
{
/**
 * 에셋 임포트 설정(Import Settings)의 기본 클래스
 */
class SE_EDITOR_API SE_ANNOTATION(=meta::Reflect) ImportSettingsBase
{
    SE_CLASS(ImportSettingsBase)

public:
    virtual ~ImportSettingsBase() = default;
};
} // namespace se::editor
