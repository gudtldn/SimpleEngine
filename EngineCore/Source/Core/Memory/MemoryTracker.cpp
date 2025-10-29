#include "Core/Memory/MemoryTracker.h"

#include <ranges>

#include "Core/Container/HashMap.h"
#include "Core/Logging/Logging.h"
#include "Core/Memory/OsMemory.h"

#include "tracy/Tracy.hpp"


namespace
{
struct LeakInfo
{
    usize size;
    std::stacktrace trace;
};

// <메모리 주소, 누수 정보>를 저장하는 Map
se::HashMap<const void*, LeakInfo> GLeakedMemoryMap;
TracyLockable(std::mutex, GMapMutex);
}

namespace se::core::memory
{
std::atomic<usize> MemoryTracker::TotalAllocated = 0;
std::atomic<usize> MemoryTracker::AllocationCount = 0;

void MemoryTracker::TrackAllocation(const void* address)
{
    const usize allocated_size = OsMemory::GetAllocatedSize(address);

#ifdef SE_DEBUG_BUILD
    {
        std::scoped_lock lock(GMapMutex);
        GLeakedMemoryMap[address] = {
            .size = allocated_size,
            .trace = std::stacktrace::current()
        };
    }
#endif

    // 메모리 트래킹 시작
    TotalAllocated.fetch_add(allocated_size, std::memory_order_relaxed);
    AllocationCount.fetch_add(1, std::memory_order_relaxed);
}

void MemoryTracker::TrackDeallocation(const void* address)
{
    const usize allocated_size = OsMemory::GetAllocatedSize(address);

#ifdef SE_DEBUG_BUILD
    std::scoped_lock lock(GMapMutex);
    GLeakedMemoryMap.Remove(address);
#endif

    // 메모리 트래킹 해제
    TotalAllocated.fetch_sub(allocated_size, std::memory_order_relaxed);
    AllocationCount.fetch_sub(1, std::memory_order_relaxed);
}

bool MemoryTracker::CheckForLeaks()
{
#ifdef SE_DEBUG_BUILD
    std::scoped_lock lock(GMapMutex);
    if (!GLeakedMemoryMap.IsEmpty())
    {
        ConsoleLog(ELogLevel::Error, "--- MEMORY LEAKS DETECTED ---");
        for (const auto& [address, info] : GLeakedMemoryMap)
        {
            ConsoleLog(ELogLevel::Error, "Leak: {} bytes at {:p}, callstack:", info.size, address);

            // stacktrace 출력
            for (const std::stacktrace_entry& entry : info.trace | std::views::reverse)
            {
                ConsoleLog(ELogLevel::Error, "    {}", entry);
            }
        }
        return true;
    }
#endif
    return false;
}
}
