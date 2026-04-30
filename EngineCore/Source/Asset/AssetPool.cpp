#include "SimpleEngine/Asset/AssetPool.h"
#include "SimpleEngine/Utility/Debug.h"

#include <utility>


namespace se
{
AssetPool::~AssetPool()
{
    // 미처리된 지연 파괴 항목들을 모두 해제
    for (auto& [ptr, destructor, _] : pending_destroy)
    {
        if (ptr && destructor)
        {
            destructor(ptr);
        }
    }
}

Optional<HandleData> AssetPool::Find(const AssetId& id) const
{
    return table.Find(id);
}

HandleData AssetPool::FindOrCreate(const AssetId& id, const TypeId& type_id, const AssetPath& asset_path)
{
    return table.FindOrCreate(id, type_id, asset_path);
}

void AssetPool::Remove(const AssetId& id)
{
    if (const Optional<HandleData> handle_data = table.Find(id))
    {
        Array<AssetPayload> deferred;
        table.EvictSlot(handle_data->index, deferred);

        const uint64 frame = table.GetCurrentFrame();
        for (AssetPayload& payload : deferred)
        {
            DeferDestroy(std::move(payload), frame);
        }
    }
}

uint32 AssetPool::CollectGarbage()
{
    Array<AssetPayload> deferred;
    const uint32 count = table.CollectGarbage(deferred);

    const uint64 frame = table.GetCurrentFrame();
    for (AssetPayload& payload : deferred)
    {
        DeferDestroy(std::move(payload), frame);
    }
    return count;
}

uint32 AssetPool::GetCount() const
{
    return table.GetCount();
}

void AssetPool::SetMemoryBudget(uint64 budget_bytes)
{
    memory_budget = budget_bytes;
}

void AssetPool::SetGraceFrames(uint64 frames)
{
    grace_frames = frames;
}

void AssetPool::SetMaxEvictionsPerFrame(uint32 count)
{
    max_evictions_per_frame = count;
}

void AssetPool::SetMaxDestructionsPerFrame(uint32 count)
{
    max_destructions_per_frame = count;
}

void AssetPool::DeferDestroy(AssetPayload payload, uint64 current_frame)
{
    if (!payload.ptr)
    {
        return;
    }

    SE_ASSERT(payload.destructor != nullptr, "AssetPool::DeferDestroy - Destructor is null for a valid asset pointer!");

    std::scoped_lock lock(pending_destroy_mutex);
    pending_destroy.Push({
        .ptr = std::exchange(payload.ptr, nullptr),
        .destructor = payload.destructor,
        .release_frame = current_frame + 1,
    });
}

void AssetPool::ProcessPendingDestroy(uint64 current_frame)
{
    ZoneScopedN("AssetPool::ProcessPendingDestroy");

    Array<AssetPayload> to_destroy;
    to_destroy.Reserve(max_destructions_per_frame);

    {
        std::scoped_lock lock(pending_destroy_mutex);

        usize write = 0;
        uint32 total_released = 0;

        for (usize read = 0; read < pending_destroy.Len(); ++read)
        {
            PendingDestroyEntry& item = pending_destroy[read];

            // 지정된 유예 프레임이 지났으므로 안전하게 실제 메모리 해제
            if (item.release_frame <= current_frame && total_released < max_destructions_per_frame)
            {
                to_destroy.Push(AssetPayload{ item.ptr, item.destructor });
                ++total_released;
            }
            else
            {
                // 아직 수명이 남은 포인터는 배열 앞쪽의 빈자리(write)로 당겨서 보존
                if (write != read)
                {
                    pending_destroy[write] = std::move(item);
                }
                ++write;
            }
        }
        pending_destroy.Truncate(write);
    }

    // 임계 구역 종료 후, 실제 소멸자 호출
    for (const auto& [ptr, destructor] : to_destroy)
    {
        destructor(ptr);
    }
}

uint32 AssetPool::EvictIfOverBudget(uint64 current_frame)
{
    ZoneScopedN("AssetPool::EvictIfOverBudget");

    // 아직 메모리 상한에 도달하지 않은 경우
    if (table.GetTotalMemoryUsage() <= memory_budget)
    {
        return 0;
    }

    // 아래 Scope 순서대로 Evict
    constexpr EScopeLayer eviction_order[] = {
        EScopeLayer::Transient,
        EScopeLayer::Scene,
        EScopeLayer::Session
    };

    Array<AssetPayload> deferred;
    uint32 total_evicted = 0;
    for (EScopeLayer target_scope : eviction_order)
    {
        // 아직 메모리 상한에 도달하지 않은 경우
        if (table.GetTotalMemoryUsage() <= memory_budget)
        {
            break;
        }

        // 한 프레임에 최대 해제할 수 있는 개수를 넘은 경우
        if (total_evicted >= max_evictions_per_frame)
        {
            break;
        }

        const uint32 remaining = max_evictions_per_frame - total_evicted;
        total_evicted += table.EvictWhere([this, target_scope, current_frame](uint32, const SlotEntry& entry)
        {
            // 스코프가 다른 경우
            if (entry.scope != target_scope)
            {
                return false;
            }

            // 아직 유예기간인 경우
            if (current_frame - entry.last_access_frame.load(std::memory_order_relaxed) < grace_frames)
            {
                return false;
            }

            // 메모리 상한에 도달하면 제거
            return table.GetTotalMemoryUsage() > memory_budget;
        }, deferred, remaining);
    }

    for (AssetPayload& payload : deferred)
    {
        DeferDestroy(std::move(payload), current_frame);
    }
    return total_evicted;
}

uint32 AssetPool::UnloadScope(EScopeLayer layer)
{
    ZoneScopedN("AssetPool::UnloadScope");

    SE_ASSERT(layer != EScopeLayer::Global, "AssetPool::UnloadScope - Global scope cannot be unloaded");

    // Release 빌드에서도 보호
    if (layer == EScopeLayer::Global)
    {
        // ReSharper disable once CppDFAUnreachableCode
        return 0;
    }

    Array<AssetPayload> deferred;
    const uint32 count = table.EvictWhere([layer](uint32, const SlotEntry& entry)
    {
        return entry.scope == layer;
    }, deferred);

    const uint64 frame = table.GetCurrentFrame();
    for (AssetPayload& payload : deferred)
    {
        DeferDestroy(std::move(payload), frame);
    }
    return count;
}
} // namespace se
