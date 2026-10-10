#pragma once

#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Core/Reflection/Registrar.h"
#include "SimpleEngine/Core/Reflection/Rtti.h"


namespace se
{
/**
 * Engine에서 사용되는 Subsystem의 기본 구조를 정의하는 인터페이스 클래스
 */
class SE_CORE_API SubsystemBase
{
public:
    SE_RTTI_ROOT()

    virtual ~SubsystemBase() = default;

public:
    /** Subsystem이 생성될 때 호출됩니다. */
    [[nodiscard]] virtual bool Initialize() = 0;

    /** Subsystem이 해제되기 직전에 호출됩니다. */
    virtual void Release() = 0;
};
}

SE_DECLARE_REFLECTION(se::SubsystemBase, SE_CORE_API)
