export module SE.Core:Memory.MemoryTracker;

import SE.Types;
import std;


namespace se::core::memory
{
/**
 * 각 메모리 할당 앞에 붙는 메타데이터 헤더
 */
export struct MemoryAllocHeader
{
    /** 사용자가 요청한 실제 데이터의 크기 */
    size_t alloc_size;
    uint32 padding;

#ifdef _DEBUG
    /** Alloc 당시의 Stacktrace */
    std::stacktrace trace;

    // 메모리 누수 체크용 linked list
    MemoryAllocHeader* next = nullptr;
    MemoryAllocHeader* prev = nullptr;
#endif

    MemoryAllocHeader(size_t size, uint8 pad)
        : alloc_size(size)
        , padding(pad)
#ifdef _DEBUG
        , trace(std::stacktrace::current())
#endif
    {
    }
};

/** MemoryAllocHeader의 크기 */
constexpr size_t HEADER_SIZE = sizeof(MemoryAllocHeader);

/**
 * 모든 메모리 할당을 추적하고 통계 및 디버깅 정보를 제공하는 정적 클래스
 */
export class MemoryTracker
{
public:
    /**
     * 새로운 메모리 할당을 추적 리스트에 추가하고 통계를 업데이트 합니다.
     * @param header_ptr 할당된 메모리의 헤더 주소
     * @param size 사용자가 요청한 데이터의 크기
     */
    static void TrackAllocation(void* header_ptr, size_t size);

    /**
     * 추적되고 있던 메모리를 추적 리스트에서 제거하고 통계를 업데이트 합니다.
     * @param header_ptr 해제될 메모리의 헤더 주소
     */
    static void TrackDeallocation(const void* header_ptr);

    /** 현재 메모리 사용량 통계를 콘솔에 출력합니다. */
    static void PrintStats();

    /**
     * 프로그램 종료 시 호출하여 해제되지 않은 메모리(누수)를 로그에 남깁니다.
     * @return 누수가 있다면 true, 없으면 false
     */
    static bool CheckForLeaks();

public:
    /** 현재 추적되고 있는 모든 메모리 Byte를 반환합니다. */
    [[nodiscard]] static size_t GetTotalAllocated() { return TotalAllocated; }

    /** 현재 추적되고 있는 모든 메모리 개수를 반환합니다. */
    [[nodiscard]] static size_t GetAllocationCount() { return AllocationCount; }

private:
    static std::atomic<size_t> TotalAllocated;
    static std::atomic<size_t> AllocationCount;
};
}
