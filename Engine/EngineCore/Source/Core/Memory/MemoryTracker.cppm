export module SE.Core:Memory.MemoryTracker;

import SE.Types;
import std;


namespace se::core::memory
{
/**
 * 메모리 추적용 헤더 정보
 */
struct TrackingHeader
{
    /** Alloc 당시의 Stacktrace */
    std::stacktrace trace;

    // 메모리 누수 체크용 linked list
    TrackingHeader* next = nullptr;
    TrackingHeader* prev = nullptr;

    TrackingHeader()
        : trace(std::stacktrace())
    {
    }
};

/**
 * 모든 메모리 할당을 추적하고 통계 및 디버깅 정보를 제공하는 정적 클래스
 */
export class MemoryTracker
{
public:
    /**
     * 새로운 메모리 할당을 추적 리스트에 추가하고 통계를 업데이트 합니다.
     * @param address 할당된 메모리의 주소
     */
    static void TrackAllocation(const void* address);

    /**
     * 추적되고 있던 메모리를 추적 리스트에서 제거하고 통계를 업데이트 합니다.
     * @param address 해제될 메모리의 주소
     */
    static void TrackDeallocation(const void* address);

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
