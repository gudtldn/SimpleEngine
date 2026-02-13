#pragma once
#include "SimpleEngine/Core/Reflection/Reflect.h"


namespace se::graphics
{
class RenderGraphBuilder;
class RGExecutionContext;

/**
 * Render Graph의 각 렌더링 단계를 정의하기 위한 인터페이스
 */
class SE_CORE_API SE_ANNOTATION(=meta::Internal) RenderPassBase
{
    SE_CLASS(RenderPassBase)

public:
    virtual ~RenderPassBase() = default;

    /**
    * Render Graph가 Compile될 때 호출됩니다.
    * 이 함수 내에서 builder를 사용하여 이 패스가 읽거나 쓰는 리소스를 선언해야 합니다.
    */
    virtual void Setup(RenderGraphBuilder& builder) = 0;

    /**
    * Render Graph가 Execute될 때 호출됩니다.
    * 이 함수 내에서 실제 렌더링 커맨드를 커맨드 버퍼에 기록해야 합니다.
    */
    virtual void Execute(RGExecutionContext& context) = 0;
};
}  // namespace se::graphics
