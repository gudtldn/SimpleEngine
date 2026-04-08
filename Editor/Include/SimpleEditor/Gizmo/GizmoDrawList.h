#pragma once

#include "SimpleEditor/EditorAPI.h"
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

    /** LINELIST 정점 2개를 추가합니다. */
    void AddLine(const GizmoVertex& v0, const GizmoVertex& v1);

    /** TRIANGLELIST 정점 3개를 추가합니다. */
    void AddTriangle(const GizmoVertex& v0, const GizmoVertex& v1, const GizmoVertex& v2);

    /** 수집된 정점을 GPU 버퍼에 업로드합니다. */
    void UploadToGpu(SDL_GPUCommandBuffer* cmd);

    [[nodiscard]] SDL_GPUBuffer* GetLineVertexBuffer() const;
    [[nodiscard]] SDL_GPUBuffer* GetTriangleVertexBuffer() const;

    [[nodiscard]] FORCE_INLINE usize GetLineVertexCount() const { return line_vertices.Len(); }
    [[nodiscard]] FORCE_INLINE usize GetTriangleVertexCount() const { return triangle_vertices.Len(); }

private:
    Array<GizmoVertex> line_vertices;     // CPU 측 라인 정점 (매 프레임 재구축)
    Array<GizmoVertex> triangle_vertices; // CPU 측 삼각형 정점 (매 프레임 재구축)

    graphics::RenderDevice* device = nullptr;
    SDL_GPUTransferBuffer* transfer_buffer = nullptr; // CPU->GPU 전송용 (라인/삼각형 공유, max 크기)
    graphics::RID line_vertex_buffer_rid;             // LINELIST 정점 버퍼
    graphics::RID triangle_vertex_buffer_rid;         // TRIANGLELIST 정점 버퍼
};
} // namespace se::editor
