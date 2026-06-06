#pragma once

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Math/Color.h"
#include "SimpleEngine/Core/Math/Math.h"
#include "SimpleEngine/Core/Subsystem/SubsystemBase.h"
#include "SimpleEngine/Graphics/Device/RID.h"

#include "SDL3/SDL_gpu.h"
#include "tracy/Tracy.hpp"

#include <mutex>


namespace se
{
// forward declaration
class RenderDevice;

/** GPU에 업로드하는 DebugLine의 정점 */
struct DebugVertex
{
    Vector3f position; // 월드 공간 위치
    LinearColor color; // Linear 색상
};

/** CPU에서 관리하는 DebugLine에 대한 정보 */
struct DebugLine
{
    Vector3 start;
    Vector3 end;
    LinearColor color = LinearColor::White();
    f32 duration = 0.0f; // 0 = 이번 프레임만
};

/**
 * 디버그 드로우 명령을 수집하고 GPU 업로드를 담당하는 서브시스템
 */
class SE_CORE_API SE_ANNOTATION(=meta::Reflect, =meta::Hidden, =meta::Transient) DebugDrawSubsystem : public SubsystemBase
{
    SE_CLASS(DebugDrawSubsystem, SubsystemBase)

public:
    static constexpr u32 MAX_DEBUG_LINES = 16384;

public:
    virtual bool Initialize() override;
    virtual void Release() override;

public:
    /** DebugLine을 이번 프레임 렌더에 등록합니다. (Thread-Safe) */
    void DrawLine(const Vector3& start, const Vector3& end, const math::LinearColor& color, f32 duration = 0.0f);

    /** DebugLine 목록을 이번 프레임 렌더에 등록합니다. (Thread-Safe) */
    void DrawLines(ArrayView<const DebugLine> lines);

    /** AABB Bounds를 월드 좌표계로 변환 후 DebugLine으로 등록합니다. (Thread-Safe) */
    void DrawAABB(const AABBf& aabb, const Matrix4x4& model, const math::LinearColor& color);

    /** 이번 프레임에 등록된 DebugLine을 GPU 버퍼에 업로드합니다. */
    void UploadToGpu(SDL_GPUCommandBuffer* cmd);

    /** 이번 프레임에 등록된 DebugLine의 개수를 가져옵니다. */
    [[nodiscard]] usize GetLineCount() const { return current_frame_lines.Len(); }

    [[nodiscard]] SDL_GPUBuffer* GetVertexBuffer() const;

private:
    // DebugLine 등록 대기열
    TracyLockable(std::mutex, pending_mutex);
    Array<DebugLine> pending_lines;

    // 이번 프레임에 렌더링할 DebugLine
    Array<DebugLine> current_frame_lines;

    // GPU 리소스
    RenderDevice* render_device = nullptr;
    RID vertex_buffer_rid;
    SDL_GPUTransferBuffer* transfer_buffer = nullptr;
};
} // namespace se

SE_DECLARE_REFLECTION(se::DebugDrawSubsystem)
