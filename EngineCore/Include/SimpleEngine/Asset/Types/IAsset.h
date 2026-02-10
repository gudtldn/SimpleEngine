#pragma once
#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Core/Reflection/Reflect.h"


namespace se::asset
{
/**
 * Asset을 나타내는 가장 기본적인 타입
 */
class SE_CORE_API IAsset
{
    SE_CLASS(IAsset)

public:
    virtual ~IAsset() = default;

    /** 이 Asset의 표시 이름을 반환합니다. (디버깅/에디터용) */
    [[nodiscard]] virtual StringView GetDisplayName() const { return {}; }
};
}  // namespace se::asset
