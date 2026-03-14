#pragma once

#include "SimpleEngine/Core/Concurrency/Common.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"


namespace se
{
/**
 * Lock-Free 고정 크기 블록 풀 할당자
 *
 * Job Payload, 코루틴 프레임 등 빈번한 할당/해제를 위해 설계되었습니다.
 * 4개의 Size Class (64/128/256/512 Byte) 별 Pool을 유지하며, Lock-Free로 할당됩니다.
 *
 * 할당 흐름:
 * 1) 요청 크기에 맞는 Size Class 결정 (유저 크기 + 헤더 16 Byte)
 * 2) TLS 캐시에서 Pop 시도 (단일 스레드, 무경합 Lock-Free)
 * 3) TLS가 비어있다면, Global Pool에서 여러 블록을 한 번에 Steal (Amortized O(1) Lock)
 * 4) Global Pool도 비어있다면, OS에서 새 Slab을 할당받아 분할
 * 5) 512B 초과 시 OS 직접 할당 (Oversized Fallback)
 *
 * 해제 흐름:
 * 1) 블록 헤더에서 Size Class 복원
 * 2) 호출 스레드의 TLS 캐시에 Push (무경합)
 * 3) TLS 캐시가 한도(MAX_CACHED_BLOCKS)를 초과하면, 절반을 떼어내어 Global Pool로 반환 (메모리 밸런싱 및 누수 방지)
 */
class SE_CORE_API JobAllocator
{
public:
    /**
     * 지정된 크기 이상의 메모리 블록을 할당합니다.
     *
     * @param in_size 필요한 유저 데이터 크기 (바이트 단위)
     * @return 할당된 메모리의 시작 포인터 (최소 16바이트 정렬 보장)
     * @note 512B 이하의 요청은 TLS 캐시에서 즉시 할당되며, 초과 시 OS 직접 할당(Fallback)으로 전환됩니다.
     */
    [[nodiscard]] static void* Allocate(usize in_size);

    /**
     * 이전에 Allocate를 통해 할당된 메모리 블록을 해제합니다.
     *
     * @param in_ptr Allocate가 반환한 메모리 포인터. nullptr인 경우 무시됩니다.
     * @note 할당한 스레드와 다른 스레드에서 해제해도 안전하며, 해제 시 호출 스레드의 TLS 캐시로 귀속됩니다.
     */
    static void Free(void* in_ptr);

    /**
     * 할당자에서 관리 중인 모든 메모리(Slab)를 OS에 반환합니다.
     * @warning 엔진 종료 시 단 한 번 호출해야 합니다. 호출 후 Allocate/Free 사용 시 UB 입니다.
     */
    static void Shutdown();
};
} // namespace se
