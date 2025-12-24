#include "Core/Memory/MemoryStats.h"

#if SE_ENABLE_MEMORY_TRACKING
#include "Utility/Debug.h"


namespace se::core
{
// static 초기화 시점에 기록할 수 있도록
std::atomic<uint32> MemoryStats::registered_count = 1;
FixedArray<MemoryTag, MemoryStats::MAX_MEMORY_TAGS> MemoryStats::tags{};
HashMap<StringName, uint32> MemoryStats::tag_lookup;

[[maybe_unused]] static const bool DefaultTagInitialized = [] static
{
    if (auto tags_span = MemoryStats::GetTags(); !tags_span.empty())
    {
        tags_span[0].name = "Untagged";
    }
    return true;
}();

std::atomic<usize> MemoryStats::total_cpu_allocated = 0;
std::atomic<usize> MemoryStats::total_gpu_allocated = 0;

// TLS 변수 (스레드별 현재 태그 ID)
thread_local uint32 CurrentThreadTagId = 0;


uint32 MemoryStats::GetOrRegisterTag(const StringName& name)
{
    std::scoped_lock lock(registry_mutex);
    return tag_lookup.Entry(name).OrInsertWith([&name = std::as_const(name)]
    {
        const uint32 new_id = registered_count.load(std::memory_order_relaxed);
        SE_ASSERT(new_id < MAX_MEMORY_TAGS, "Memory tags capacity exceeded!");

        tags[new_id].name = name;

        registered_count.store(new_id + 1, std::memory_order_release);
        return new_id;
    });
}

uint32 MemoryStats::SetCurrentTag(uint32 tag_id)
{
    return std::exchange(CurrentThreadTagId, tag_id);
}

uint32 MemoryStats::GetCurrentTag()
{
    return CurrentThreadTagId;
}

void MemoryStats::TrackAlloc(uint32 tag_id, usize size)
{
    if (SE_ENSURE(tag_id < registered_count.load(std::memory_order_acquire), "Invalid memory tag ID!"))
    {
        total_cpu_allocated.fetch_add(size, std::memory_order_relaxed);
    }
    tags[tag_id].cpu_allocated.fetch_add(size, std::memory_order_relaxed);
}

void MemoryStats::TrackFree(uint32 tag_id, usize size)
{
    if (SE_ENSURE(tag_id < registered_count.load(std::memory_order_acquire), "Invalid memory tag ID!"))
    {
        total_cpu_allocated.fetch_sub(size, std::memory_order_relaxed);
    }
    tags[tag_id].cpu_allocated.fetch_sub(size, std::memory_order_relaxed);
}

void MemoryStats::TrackGpuAlloc(uint32 tag_id, usize size)
{
    if (SE_ENSURE(tag_id < registered_count.load(std::memory_order_acquire), "Invalid memory tag ID!"))
    {
        total_gpu_allocated.fetch_add(size, std::memory_order_relaxed);
    }
    tags[tag_id].gpu_allocated.fetch_add(size, std::memory_order_relaxed);
}

void MemoryStats::TrackGpuFree(uint32 tag_id, usize size)
{
    if (SE_ENSURE(tag_id < registered_count.load(std::memory_order_acquire), "Invalid memory tag ID!"))
    {
        total_gpu_allocated.fetch_sub(size, std::memory_order_relaxed);
    }
    tags[tag_id].gpu_allocated.fetch_sub(size, std::memory_order_relaxed);
}

std::span<MemoryTag> MemoryStats::GetTags()
{
    return { tags.begin(), registered_count.load(std::memory_order_acquire) };
}

usize MemoryStats::GetTotalCpuAllocated()
{
    return total_cpu_allocated.load(std::memory_order_relaxed);
}

usize MemoryStats::GetTotalGpuAllocated()
{
    return total_gpu_allocated.load(std::memory_order_relaxed);
}
}  // namespace se::core
#endif
