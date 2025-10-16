#pragma once

#include "SimpleEngine/Core/Containers/Containers.h"
#include "SimpleEngine/Reflection/TypeId.h"


namespace se::core
{
/**
 * Engine에서 사용되는 Subsystem의 기본 구조를 정의하는 인터페이스 클래스
 * @note Subsystem구현은 ISubsystem을 사용해 주세요!
 */
class ISubsystemBase
{
public:
    virtual ~ISubsystemBase() = default;

    [[nodiscard]] virtual bool Initialize() = 0;
    virtual void Release() = 0;

    virtual vector<reflection::TypeId> GetDependencies() const = 0;
};
}
