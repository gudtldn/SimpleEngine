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

#ifdef _DEBUG
    /** Alloc 당시의 코드 위치 (파일명, 함수명, 라인넘버) */
    std::source_location loc;

    // 메모리 누수 체크용 linked list
    MemoryAllocHeader* next;
    MemoryAllocHeader* prev;
#endif
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
     * @param block 할당된 전체 블록의 시작 주소 (헤더 포함)
     * @param size 사용자가 요청한 데이터의 크기
     * @param loc 할당이 발생한 소스 코드 위치
     */
    static void TrackAllocation(void* block, size_t size, const std::source_location& loc);

    /**
     * 메모리 해제를 추적 리스트에서 제거하고 통계를 업데이트 합니다.
     * @param block 해제될 전체 블록의 시작 주소 (헤더 포함)
     */
    static void TrackDeallocation(void* block);

    /** 현재 메모리 사용량 통계를 콘솔에 출력합니다. */
    static void PrintStats();

    /**
     * 프로그램 종료 시 호출하여 해제되지 않은 메모리(누수)를 로그에 남깁니다.
     * @return 누수가 있다면 true, 없으면 false
     */
    static bool CheckForLeaks();

public:
    /** 현재 할당된 모든 메모리 Byte를 반환합니다. */
    [[nodiscard]] static size_t GetTotalAllocated() { return TotalAllocated; }

    /** 현재 할당된 모든 메모리 개수를 반환합니다. */
    [[nodiscard]] static size_t GetAllocationCount() { return AllocationCount; }

private:
    static std::atomic<size_t> TotalAllocated;
    static std::atomic<size_t> AllocationCount;
};
}
