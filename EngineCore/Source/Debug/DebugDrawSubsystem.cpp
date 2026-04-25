#include "SimpleEngine/Debug/DebugDrawSubsystem.h"

#include "SimpleEngine/Core/Logging/Logging.h"
#include "SimpleEngine/Core/Subsystem/SubsystemRegistration.h"
#include "SimpleEngine/Debug/DebugDraw.h"
#include "SimpleEngine/Graphics/RenderSubsystem.h"
#include "SimpleEngine/Graphics/Device/RenderDevice.h"
#include "SimpleEngine/Utility/SubsystemUtils.h"


namespace se
{
SE_REGISTER_SUBSYSTEM(DebugDrawSubsystem)
    .DependsOn<RenderSubsystem>();

SE_BEGIN_REFLECT(DebugDrawSubsystem, meta::Internal)
SE_END_REFLECT(DebugDrawSubsystem)

bool DebugDrawSubsystem::Initialize()
{
    render_device = &GetSubsystemChecked<RenderSubsystem>().GetRenderDevice();

    constexpr uint32 buffer_size = static_cast<usize>(MAX_DEBUG_LINES * 2) * sizeof(DebugVertex);
    vertex_buffer_rid = render_device->CreateBuffer({
        .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
        .size = buffer_size,
    }, "DebugDraw_VertexBuffer");

    if (!render_device->IsValidBuffer(vertex_buffer_rid))
    {
        ConsoleLog(ELogLevel::Error, "DebugDrawSubsystem: Failed to create vertex buffer");
        return false;
    }

    constexpr SDL_GPUTransferBufferCreateInfo transfer_buffer_info = {
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        .size = buffer_size,
    };
    transfer_buffer = SDL_CreateGPUTransferBuffer(render_device->GetRawDevice(), &transfer_buffer_info);

    if (!transfer_buffer)
    {
        ConsoleLog(ELogLevel::Error, "DebugDrawSubsystem: Failed to create transfer buffer: {}", SDL_GetError());
        return false;
    }

    return true;
}

void DebugDrawSubsystem::Release()
{
    if (render_device)
    {
        if (transfer_buffer)
        {
            SDL_ReleaseGPUTransferBuffer(
                render_device->GetRawDevice(),
                std::exchange(transfer_buffer, nullptr)
            );
        }

        if (render_device->IsValidBuffer(vertex_buffer_rid))
        {
            render_device->DestroyBuffer(
                std::exchange(vertex_buffer_rid, {})
            );
        }
    }

    render_device = nullptr;
}

void DebugDrawSubsystem::DrawLine(
    const Vector3& start,
    const Vector3& end,
    const LinearColor& color,
    float duration
)
{
    std::scoped_lock lock{ pending_mutex };
    if (pending_lines.Len() >= MAX_DEBUG_LINES)
    {
        return;
    }
    pending_lines.Push({ .start = start, .end = end, .color = color, .duration = duration });
}

void DebugDrawSubsystem::DrawLines(ArrayView<const DebugLine> lines)
{
    std::scoped_lock lock{ pending_mutex };
    for (const DebugLine& line : lines)
    {
        if (pending_lines.Len() >= MAX_DEBUG_LINES)
        {
            return;
        }
        pending_lines.Push(line);
    }
}

void DebugDrawSubsystem::UploadToGpu(SDL_GPUCommandBuffer* cmd)
{
    // 현재 프레임에 등록된 DebugLine을 Swap (이후 DrawLine 호출은 다음 프레임에 반영)
    {
        std::scoped_lock lock{ pending_mutex };
        std::swap(current_frame_lines, pending_lines);
        pending_lines.Clear();
    }

    const usize line_count = current_frame_lines.Len();
    if (line_count == 0)
    {
        return;
    }

    const usize data_size = line_count * 2 * sizeof(DebugVertex);

    // TransferBuffer에 업로드 (cycle을 true로 설정하여, 이전 프레임의 버퍼를 대기하지 않음)
    if (DebugVertex* mapped = static_cast<DebugVertex*>(SDL_MapGPUTransferBuffer(render_device->GetRawDevice(), transfer_buffer, true)))
    {
        for (usize i = 0; i < line_count; ++i)
        {
            const DebugLine& line = current_frame_lines[i];
            mapped[(i * 2) + 0] = {
                .position = static_cast<Vector3f>(line.start),
                .color = line.color,
            };
            mapped[(i * 2) + 1] = {
                .position = static_cast<Vector3f>(line.end),
                .color = line.color,
            };
        }
        SDL_UnmapGPUTransferBuffer(render_device->GetRawDevice(), transfer_buffer);
    }
    else
    {
        ConsoleLog(ELogLevel::Warning, "DebugDrawSubsystem: Failed to map transfer buffer: {}", SDL_GetError());
        return;
    }

    // TransferBuffer에서 VertexBuffer로 복사
    SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(cmd);
    const SDL_GPUTransferBufferLocation src = { .transfer_buffer = transfer_buffer, .offset = 0 };
    const SDL_GPUBufferRegion dst = {
        .buffer = GetVertexBuffer(),
        .offset = 0,
        .size = static_cast<uint32>(data_size),
    };
    SDL_UploadToGPUBuffer(copy_pass, &src, &dst, false);
    SDL_EndGPUCopyPass(copy_pass);
}

SDL_GPUBuffer* DebugDrawSubsystem::GetVertexBuffer() const
{
    if (const auto resource = render_device->GetBuffer(vertex_buffer_rid))
    {
        return resource->handle;
    }
    return nullptr;
}

void DrawDebugLine(const Vector3& start, const Vector3& end, const LinearColor& color, float duration)
{
    if (DebugDrawSubsystem* debug = GetSubsystem<DebugDrawSubsystem>())
    {
        debug->DrawLine(start, end, color, duration);
    }
}

void DrawDebugLines(ArrayView<const DebugLine> lines)
{
    if (DebugDrawSubsystem* debug = GetSubsystem<DebugDrawSubsystem>())
    {
        debug->DrawLines(lines);
    }
}

void DrawDebugRay(const Ray& ray, double length, const LinearColor& color, float duration)
{
    DrawDebugLine(ray.origin, ray.GetPoint(length), color, duration);
}
} // namespace se
