#include "SimpleEngine/Core/Memory/MemoryStats.h"

#if SE_ENABLE_MEMORY_TRACKING
#include "SimpleEngine/Utility/Debug.h"


namespace se
{
namespace
{
[[maybe_unused]] const bool DefaultTagInitialized = [] static
{
    if (const auto tags_view = MemoryStats::GetTags(); !tags_view.IsEmpty())
    {
        tags_view[0].name = "Untagged";
    }
    return true;
}();

// TLS 변수 (스레드별 현재 태그 ID)
thread_local u32 current_thread_tag_id = 0;
} // namespace

// static 초기화 시점에 기록할 수 있도록
std::atomic<u32> MemoryStats::registered_count = 1;
FixedArray<MemoryTag, MemoryStats::MAX_MEMORY_TAGS> MemoryStats::tags{};
HashMap<StringName, u32> MemoryStats::tag_lookup;

std::atomic<usize> MemoryStats::total_cpu_allocated = 0;
std::atomic<usize> MemoryStats::total_gpu_allocated = 0;


u32 MemoryStats::GetOrRegisterTag(const StringName& name)
{
    std::scoped_lock lock{ registry_mutex };
    return tag_lookup.Entry(name).OrInsertWith([&name = std::as_const(name)]
    {
        const u32 new_id = registered_count.load(std::memory_order_relaxed);
        SE_ASSERT(new_id < MAX_MEMORY_TAGS, "Memory tags capacity exceeded!");

        tags[new_id].name = name;

        registered_count.store(new_id + 1, std::memory_order_release);
        return new_id;
    });
}

u32 MemoryStats::SetCurrentTag(u32 tag_id)
{
    return std::exchange(current_thread_tag_id, tag_id);
}

u32 MemoryStats::GetCurrentTag()
{
    return current_thread_tag_id;
}

void MemoryStats::TrackAlloc(u32 tag_id, usize size)
{
    if (SE_ENSURE(tag_id < registered_count.load(std::memory_order_acquire), "Invalid memory tag ID!"))
    {
        total_cpu_allocated.fetch_add(size, std::memory_order_relaxed);
    }
    tags[tag_id].cpu_allocated.fetch_add(size, std::memory_order_relaxed);
}

void MemoryStats::TrackFree(u32 tag_id, usize size)
{
    if (SE_ENSURE(tag_id < registered_count.load(std::memory_order_acquire), "Invalid memory tag ID!"))
    {
        total_cpu_allocated.fetch_sub(size, std::memory_order_relaxed);
    }
    tags[tag_id].cpu_allocated.fetch_sub(size, std::memory_order_relaxed);
}

void MemoryStats::TrackGpuAlloc(u32 tag_id, usize size)
{
    if (SE_ENSURE(tag_id < registered_count.load(std::memory_order_acquire), "Invalid memory tag ID!"))
    {
        total_gpu_allocated.fetch_add(size, std::memory_order_relaxed);
    }
    tags[tag_id].gpu_allocated.fetch_add(size, std::memory_order_relaxed);
}

void MemoryStats::TrackGpuFree(u32 tag_id, usize size)
{
    if (SE_ENSURE(tag_id < registered_count.load(std::memory_order_acquire), "Invalid memory tag ID!"))
    {
        total_gpu_allocated.fetch_sub(size, std::memory_order_relaxed);
    }
    tags[tag_id].gpu_allocated.fetch_sub(size, std::memory_order_relaxed);
}

ArrayView<MemoryTag> MemoryStats::GetTags()
{
    return { tags.Data(), registered_count.load(std::memory_order_acquire) };
}

usize MemoryStats::GetTotalCpuAllocated()
{
    return total_cpu_allocated.load(std::memory_order_relaxed);
}

usize MemoryStats::GetTotalGpuAllocated()
{
    return total_gpu_allocated.load(std::memory_order_relaxed);
}
} // namespace se
#endif
