#pragma once

#include "SimpleEditor/EditorCommon.h"
#include "SimpleEditor/Gizmo/GizmoVertex.h"

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Graphics/Device/RID.h"

#include "SDL3/SDL_gpu.h"

// forward declaration
namespace se::graphics{ class RenderDevice; }


namespace se::editor
{
/**
 * Immediate Mode 기즈모 드로우 리스트
 *
 * 매 프레임 Clear() 후 AddLine/AddTriangle로 프리미티브를 수집하고,
 * UploadToGpu()로 GPU 버퍼에 업로드합니다. GizmoPass가 이 버퍼를 렌더링합니다.
 */
class SE_EDITOR_API GizmoDrawList
{
public:
    /** 한 프레임에 제출 가능한 최대 라인 수 */
    static constexpr uint32 MAX_LINES = 1024;

    /** 한 프레임에 제출 가능한 최대 삼각형 수 */
    static constexpr uint32 MAX_TRIANGLES = 1024;

public:
    GizmoDrawList(graphics::RenderDevice& in_device);
    ~GizmoDrawList();

    GizmoDrawList(const GizmoDrawList&) = delete;
    GizmoDrawList& operator=(const GizmoDrawList&) = delete;
    GizmoDrawList(GizmoDrawList&&) = delete;
    GizmoDrawList& operator=(GizmoDrawList&&) = delete;

public:
    /** 프레임 시작 시 이전 프레임의 정점 데이터를 비웁니다. */
    void Clear();

    /**
     * LINELIST 정점 2개를 추가합니다.
     * @param v0 라인 시작 정점
     * @param v1 라인 끝 정점
     */
    void AddLine(const GizmoVertex& v0, const GizmoVertex& v1);

    /**
     * TRIANGLELIST 정점 3개를 추가합니다.
     * @param v0 삼각형 첫 번째 정점
     * @param v1 삼각형 두 번째 정점
     * @param v2 삼각형 세 번째 정점
     */
    void AddTriangle(const GizmoVertex& v0, const GizmoVertex& v1, const GizmoVertex& v2);

    /**
     * 수집된 정점을 GPU 버퍼에 업로드합니다.
     * @param cmd 전송 커맨드를 기록할 GPU 커맨드 버퍼
     */
    void UploadToGpu(SDL_GPUCommandBuffer* cmd);

    /** 업로드된 LINELIST GPU 정점 버퍼를 반환합니다 (GizmoPass에서 바인드용) */
    [[nodiscard]] SDL_GPUBuffer* GetLineVertexBuffer() const;
    /** 업로드된 TRIANGLELIST GPU 정점 버퍼를 반환합니다 (GizmoPass에서 바인드용) */
    [[nodiscard]] SDL_GPUBuffer* GetTriangleVertexBuffer() const;

    /** 현재 수집된 라인 정점 수 (= 라인 수 * 2) */
    [[nodiscard]] FORCE_INLINE usize GetLineVertexCount() const { return line_vertices.Len(); }
    /** 현재 수집된 삼각형 정점 수 (= 삼각형 수 * 3) */
    [[nodiscard]] FORCE_INLINE usize GetTriangleVertexCount() const { return triangle_vertices.Len(); }

    /** 기즈모 월드 중심 좌표 (GPU 스케일 계산용, 프레임별 갱신) */
    FORCE_INLINE void SetCenter(const Vector3& in_center) { center = in_center; }
    [[nodiscard]] FORCE_INLINE const Vector3& GetCenter() const { return center; }

    /** 포커스 뷰포트의 카메라 -> 기즈모 방향 (Rotation quarter-ring용, 프레임별 갱신) */
    FORCE_INLINE void SetDirectionToWidget(const Vector3& in_dir) { direction_to_widget = in_dir; }
    [[nodiscard]] FORCE_INLINE const Vector3& GetDirectionToWidget() const { return direction_to_widget; }

    /** pick_id 설정 (Color Picking용, 프레임별 갱신) */
    FORCE_INLINE void SetPickId(uint32 in_pick_id) { current_pick_id = in_pick_id; }
    [[nodiscard]] FORCE_INLINE uint32 GetPickId() const { return current_pick_id; }

private:
    Array<GizmoVertex> line_vertices;                 // CPU 측 라인 정점 (매 프레임 재구축)
    Array<GizmoVertex> triangle_vertices;             // CPU 측 삼각형 정점 (매 프레임 재구축)
    Vector3 center = Vector3::Zero();                 // 기즈모 월드 중심 (매 프레임 재구축)
    Vector3 direction_to_widget = Vector3::Forward(); // 카메라 -> 기즈모 방향 (매 프레임 재구축)
    uint32 current_pick_id = 0;                       // 현재 선택된 기즈모 축 (Picking용)

    graphics::RenderDevice* device = nullptr;
    SDL_GPUTransferBuffer* transfer_buffer = nullptr; // CPU->GPU 전송용 (라인/삼각형 공유, max 크기)
    graphics::RID line_vertex_buffer_rid;             // LINELIST 정점 버퍼
    graphics::RID triangle_vertex_buffer_rid;         // TRIANGLELIST 정점 버퍼
};
} // namespace se::editor
