#pragma once

#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Core/Reflection/Reflect.h"


namespace se::graphics
{
class RGExecutionContext;
class RGSetupContext;

/**
 * 패스가 사용하는 GPU 큐 타입입니다.
 * 현재는 어노테이션 용도이며, 향후 멀티 큐 실행 시 Executor가 이 값을 사용합니다.
 */
enum class ERGPassQueue : uint8
{
    Graphics,  // SDL_BeginGPURenderPass
    Compute,   // SDL_BeginGPUComputePass
    Transfer,  // SDL_BeginGPUCopyPass
};

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
    * 이 함수 내에서 context를 사용하여 이 패스가 읽거나 쓰는 리소스를 선언해야 합니다.
    */
    virtual void Setup(RGSetupContext& context) = 0;

    /**
    * Render Graph가 Execute될 때 호출됩니다.
    * 이 함수 내에서 실제 렌더링 커맨드를 커맨드 버퍼에 기록해야 합니다.
    */
    virtual void Execute(RGExecutionContext& context) = 0;

    /**
     * 이 패스가 사용하는 GPU 큐 타입을 반환합니다.
     * 기본값은 Graphics이며, Compute/Transfer 패스는 이 함수를 override합니다.
     */
    [[nodiscard]] virtual ERGPassQueue GetQueueType() const { return ERGPassQueue::Graphics; }
};
} // namespace se::graphics
