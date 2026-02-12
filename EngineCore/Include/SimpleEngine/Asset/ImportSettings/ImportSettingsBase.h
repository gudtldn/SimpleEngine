#pragma once
#include "SimpleEngine/Core/Serialization/Archive_DEPRECATED.h"
#include "SimpleEngine/Core/Reflection/Reflect.h"


namespace se::asset
{
/**
 * 에셋 임포트 설정(Import Settings)의 기본 클래스
 */
class SE_CORE_API ImportSettingsBase
{
    SE_CLASS(ImportSettingsBase)

public:
    virtual ~ImportSettingsBase() = default;

    /** ImportSettings를 Archive로 직렬화합니다. */
    virtual void Serialize(Archive_DEPRECATED& ar) = 0;

public:
    friend void Serialize(Archive_DEPRECATED& ar, ImportSettingsBase& settings)
    {
        settings.Serialize(ar);
    }
};
}  // namespace se::asset
