#include "SimpleEditor/Gizmo/GizmoDrawList.h"

#include "SimpleEngine/Core/Logging/Logging.h"
#include "SimpleEngine/Core/Logging/LogLevel.h"
#include "SimpleEngine/Graphics/Device/RenderDevice.h"

#include <memory>
#include <utility>


namespace se::editor
{
GizmoDrawList::GizmoDrawList(graphics::RenderDevice& in_device)
    : device(std::addressof(in_device))
{
    // 라인 정점 버퍼 생성 (LINELIST용, 라인 1개 = 정점 2개)
    constexpr uint32 line_vertex_size = sizeof(GizmoVertex) * MAX_LINES * 2;
    line_vertex_buffer_rid = device->CreateBuffer({
        .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
        .size = line_vertex_size,
    }, "Gizmo_LineVertexBuffer");
    SE_ASSERT_RELEASE(device->IsValidBuffer(line_vertex_buffer_rid));

    // 삼각형 정점 버퍼 생성 (TRIANGLELIST용, 삼각형 1개 = 정점 3개)
    constexpr uint32 triangle_vertex_size = sizeof(GizmoVertex) * MAX_TRIANGLES * 3;
    triangle_vertex_buffer_rid = device->CreateBuffer({
        .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
        .size = triangle_vertex_size,
    }, "Gizmo_TriVertexBuffer");
    SE_ASSERT_RELEASE(device->IsValidBuffer(triangle_vertex_buffer_rid));

    // 전송 버퍼: 라인/삼각형 중 큰 쪽 크기로 할당 (순차 재사용)
    constexpr SDL_GPUTransferBufferCreateInfo create_info = {
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        .size = std::max(line_vertex_size, triangle_vertex_size),
    };
    transfer_buffer = SDL_CreateGPUTransferBuffer(device->GetRawDevice(), &create_info);
    SE_ASSERT_RELEASE(transfer_buffer);
}

GizmoDrawList::~GizmoDrawList()
{
    Clear();

    if (device->IsValidBuffer(line_vertex_buffer_rid))
    {
        device->DestroyBuffer(std::exchange(line_vertex_buffer_rid, {}));
    }

    if (device->IsValidBuffer(triangle_vertex_buffer_rid))
    {
        device->DestroyBuffer(std::exchange(triangle_vertex_buffer_rid, {}));
    }

    if (transfer_buffer)
    {
        SDL_ReleaseGPUTransferBuffer(device->GetRawDevice(), std::exchange(transfer_buffer, nullptr));
    }
}

void GizmoDrawList::Clear()
{
    current_pick_id = 0;

    line_vertices.Clear();
    triangle_vertices.Clear();
}

void GizmoDrawList::AddLine(const GizmoVertex& v0, const GizmoVertex& v1)
{
    // line_vertices.Len() >= MAX_LINES * 2
    if (std::cmp_greater_equal(line_vertices.Len(), MAX_LINES * 2))
    {
        return;
    }

    auto apply_pick_id = [this](GizmoVertex v) { v.pick_id = current_pick_id; return v; };
    line_vertices.Push(apply_pick_id(v0));
    line_vertices.Push(apply_pick_id(v1));
}

void GizmoDrawList::AddTriangle(const GizmoVertex& v0, const GizmoVertex& v1, const GizmoVertex& v2)
{
    // triangle_vertices.Len() >= MAX_TRIANGLES * 3
    if (std::cmp_greater_equal(triangle_vertices.Len(), MAX_TRIANGLES * 3))
    {
        return;
    }

    auto apply_pick_id = [this](GizmoVertex v) { v.pick_id = current_pick_id; return v; };
    triangle_vertices.Push(apply_pick_id(v0));
    triangle_vertices.Push(apply_pick_id(v1));
    triangle_vertices.Push(apply_pick_id(v2));
}

void GizmoDrawList::UploadToGpu(SDL_GPUCommandBuffer* cmd)
{
    // Upload Line Vertex
    const uint32 line_vertex_size = static_cast<uint32>(line_vertices.Len() * sizeof(GizmoVertex));
    if (line_vertex_size > 0)
    {
        // cycle=true: 이전 프레임의 전송이 아직 진행 중이면 새 메모리 할당
        if (void* mapped = SDL_MapGPUTransferBuffer(device->GetRawDevice(), transfer_buffer, true))
        {
            std::memcpy(mapped, line_vertices.Data(), line_vertex_size);
            SDL_UnmapGPUTransferBuffer(device->GetRawDevice(), transfer_buffer);
        }
        else
        {
            ConsoleLog(ELogLevel::Error, "GizmoDrawList: Failed to map transfer buffer: {}", SDL_GetError());
            return;
        }

        SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(cmd);
        {
            const SDL_GPUTransferBufferLocation src = {
                .transfer_buffer = transfer_buffer,
                .offset = 0
            };

            const SDL_GPUBufferRegion dst = {
                .buffer = GetLineVertexBuffer(),
                .offset = 0,
                .size = line_vertex_size,
            };

            SDL_UploadToGPUBuffer(copy_pass, &src, &dst, false);
        }
        SDL_EndGPUCopyPass(copy_pass);
    }

    // Upload Triangle Vertex
    const uint32 triangle_vertex_size = static_cast<uint32>(triangle_vertices.Len() * sizeof(GizmoVertex));
    if (triangle_vertex_size > 0)
    {
        // cycle=true: Line Vertex가 아직 업로드 중 이라면 새 메모리 할당
        if (void* mapped = SDL_MapGPUTransferBuffer(device->GetRawDevice(), transfer_buffer, true))
        {
            std::memcpy(mapped, triangle_vertices.Data(), triangle_vertex_size);
            SDL_UnmapGPUTransferBuffer(device->GetRawDevice(), transfer_buffer);
        }
        else
        {
            ConsoleLog(ELogLevel::Error, "GizmoDrawList: Failed to map transfer buffer: {}", SDL_GetError());
            return;
        }

        SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(cmd);
        {
            const SDL_GPUTransferBufferLocation src = {
                .transfer_buffer = transfer_buffer,
                .offset = 0
            };

            const SDL_GPUBufferRegion dst = {
                .buffer = GetTriangleVertexBuffer(),
                .offset = 0,
                .size = triangle_vertex_size,
            };

            SDL_UploadToGPUBuffer(copy_pass, &src, &dst, false);
        }
        SDL_EndGPUCopyPass(copy_pass);
    }
}

SDL_GPUBuffer* GizmoDrawList::GetLineVertexBuffer() const
{
    if (const auto resource = device->GetBuffer(line_vertex_buffer_rid))
    {
        return resource->handle;
    }
    return nullptr;
}

SDL_GPUBuffer* GizmoDrawList::GetTriangleVertexBuffer() const
{
    if (const auto resource = device->GetBuffer(triangle_vertex_buffer_rid))
    {
        return resource->handle;
    }
    return nullptr;
}
} // namespace se::editor
