module SE.Core;
import :Memory.MemoryTracker;

import SE.Core;


namespace
{
#ifdef _DEBUG
se::core::memory::MemoryAllocHeader* ListHead = nullptr;
std::mutex ListMutex;
#endif
}

namespace se::core::memory
{
void MemoryTracker::TrackAllocation(
    [[maybe_unused]] void* header_ptr,
    size_t size
)
{
    // 메모리 트래킹 시작
    TotalAllocated.fetch_add(size, std::memory_order_relaxed);
    AllocationCount.fetch_add(1, std::memory_order_relaxed);

#ifdef _DEBUG
    MemoryAllocHeader* header = static_cast<MemoryAllocHeader*>(header_ptr);
    header->trace = std::stacktrace::current();
    header->next = ListHead;
    header->prev = nullptr;

    // 전역 리스트에 추가
    std::lock_guard lock(ListMutex);
    if (ListHead != nullptr)
    {
        ListHead->prev = header;
    }
    ListHead = header;
#endif
}

void MemoryTracker::TrackDeallocation(const void* header_ptr)
{
    const MemoryAllocHeader* header = static_cast<const MemoryAllocHeader*>(header_ptr);

    // 메모리 트래킹 해제
    TotalAllocated.fetch_sub(header->alloc_size, std::memory_order_relaxed);
    AllocationCount.fetch_sub(1, std::memory_order_relaxed);

#ifdef _DEBUG
    // 전역 리스트에서 제거
    std::scoped_lock lock(ListMutex);
    if (header->prev)
    {
        header->prev->next = header->next;
    }
    if (header->next)
    {
        header->next->prev = header->prev;
    }
    if (ListHead == header)
    {
        ListHead = header->next;
    }
#endif
}

void MemoryTracker::PrintStats()
{
    ConsoleLog(ELogLevel::Info, u8"Total Allocated: {} bytes", TotalAllocated.load());
    ConsoleLog(ELogLevel::Info, u8"Allocation Count: {}", AllocationCount.load());
}

bool MemoryTracker::CheckForLeaks()
{
#ifdef _DEBUG
    std::scoped_lock lock(ListMutex);

    if (ListHead)
    {
        ConsoleLog(ELogLevel::Error, u8"--- MEMORY LEAKS DETECTED ---");
        const MemoryAllocHeader* current = ListHead;
        while (current)
        {
            ConsoleLog(ELogLevel::Error, u8"Leak: {} bytes, allocated trace:", current->alloc_size);

            // stacktrace 출력
            for (const std::stacktrace_entry& entry : current->trace | std::views::reverse)
            {
                ConsoleLog(ELogLevel::Error, u8"    {}", entry);
            }
            current = current->next;
        }
        ConsoleLog(ELogLevel::Error, u8"-----------------------------");
        return true;
    }
#endif

    ConsoleLog(ELogLevel::Info, u8"No memory leaks detected.");
    return false;
}

std::atomic<size_t> MemoryTracker::TotalAllocated = 0;
std::atomic<size_t> MemoryTracker::AllocationCount = 0;
}
