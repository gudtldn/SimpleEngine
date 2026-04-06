#pragma once

#include "SimpleEngine/Core/HAL/PlatformTypes.h"


namespace se
{
/**
 * 엔진에서 발생하는 에러의 기본 인터페이스
 */
class SE_CORE_API IError
{
public:
    virtual ~IError() = default;

    /** 사용자에게 보여줄 에러 설명을 반환합니다. */
    [[nodiscard]] virtual const char* What() const noexcept = 0;

    /**
     * 이 에러의 근본 원인이 되는 에러를 반환합니다.
     * @return 소스 에러에 대한 포인터를 반환하며, 없는 경우 nullptr를 반환합니다.
     */
    [[nodiscard]] virtual const IError* Source() const noexcept = 0;
};
} // namespace se
