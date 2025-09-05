module SE.Core;
import :Memory.MemoryTracker;

import SE.Core;
import SE.Utility;


namespace
{
#ifdef _DEBUG
se::core::memory::MemoryAllocHeader* ListHead = nullptr;
std::mutex ListMutex;
#endif
}

namespace se::core::memory
{
void MemoryTracker::TrackAllocation(void* block, size_t size, const std::source_location& loc)
{
    // 메모리 트레킹 시작
    TotalAllocated.fetch_add(size, std::memory_order_relaxed);
    AllocationCount.fetch_add(1, std::memory_order_relaxed);

    if constexpr (utility::IS_DEBUG_BUILD)
    {
        MemoryAllocHeader* header = static_cast<MemoryAllocHeader*>(block);
        header->loc = loc;
        header->next = ListHead;
        header->prev = nullptr;

        // 전역 리스트에 추가
        std::lock_guard lock(ListMutex);
        if (ListHead != nullptr)
        {
            ListHead->prev = header;
        }
        ListHead = header;
    }
}

void MemoryTracker::TrackDeallocation(void* block)
{
    const MemoryAllocHeader* header = static_cast<MemoryAllocHeader*>(block);

    // 메모리 트레킹 해제
    TotalAllocated.fetch_sub(header->alloc_size, std::memory_order_relaxed);
    AllocationCount.fetch_sub(1, std::memory_order_relaxed);

    if constexpr (utility::IS_DEBUG_BUILD)
    {
        // 전역 리스트에서 제거
        std::lock_guard lock(ListMutex);
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
    }
}

void MemoryTracker::PrintStats()
{
    ConsoleLog(ELogLevel::Info, u8"Total Allocated: {} bytes", TotalAllocated.load());
    ConsoleLog(ELogLevel::Info, u8"Allocation Count: {}", AllocationCount.load());
}

void MemoryTracker::CheckForLeaks()
{
    if constexpr (utility::IS_DEBUG_BUILD)
    {
        std::lock_guard lock(ListMutex);
        if (ListHead == nullptr)
        {
            ConsoleLog(ELogLevel::Info, u8"No memory leaks detected.");
        }
        else
        {
            ConsoleLog(ELogLevel::Error, u8"--- MEMORY LEAKS DETECTED ---");
            const MemoryAllocHeader* current = ListHead;
            while (current)
            {
                const std::source_location& loc = current->loc;
                ConsoleLog(
                    ELogLevel::Error,
                    u8"Leak: {} bytes, allocated at {}:{}",
                    current->alloc_size, loc.file_name(), loc.line()
                );
                current = current->next;
            }
            ConsoleLog(ELogLevel::Error, u8"-----------------------------");
        }
    }
}

std::atomic<size_t> MemoryTracker::TotalAllocated = 0;
std::atomic<size_t> MemoryTracker::AllocationCount = 0;
}
