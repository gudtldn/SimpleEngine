#pragma once

#include <atomic>
#include <mutex>

#include "SimpleEngine/Core/Container/ArrayView.h"
#include "SimpleEngine/Core/Container/FixedArray.h"
#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Core/Memory/MemoryConfig.h"
#include "SimpleEngine/Core/Types/StringName.h"
#include "SimpleEngine/Utility/Common.h"

#include "tracy/Tracy.hpp"


namespace se
{
/**
 * 메모리 카테고리 태그 정보를 담는 구조체
 */
struct MemoryTag
{
    // 태그의 식별 이름
    StringName name;

    // 할당된 CPU 메모리 총량 (byte)
    std::atomic<usize> cpu_allocated = 0;

    // 할당된 GPU 메모리 총량 (byte)
    std::atomic<usize> gpu_allocated = 0;
};

/**
 * 전역 메모리 통계 및 태그를 관리하는 정적 클래스
 */
class SE_CORE_API MemoryStats
{
public:
#if SE_ENABLE_MEMORY_TRACKING
    /**
     * 태그 이름으로 ID를 조회하거나, 없으면 새로 등록합니다.
     * @param name 등록하거나 조회할 태그의 이름
     * @return 등록된(혹은 조회된) 태그의 고유 ID
     */
    [[nodiscard]] static uint32 GetOrRegisterTag(const StringName& name);

    /**
     * 현재 스레드의 활성 메모리 태그를 설정합니다.
     * @param tag_id 설정할 태그의 ID
     * @return 이전에 설정되어 있던 태그 ID
     */
    static uint32 SetCurrentTag(uint32 tag_id);

    /**
     * 현재 스레드에 설정된 활성 메모리 태그 ID를 반환합니다.
     * @return 현재 태그 ID
     */
    static uint32 GetCurrentTag();

    /**
     * CPU 메모리 할당을 기록합니다.
     * @param tag_id 할당에 사용된 태그 ID
     * @param size 할당된 크기 (바이트)
     */
    static void TrackAlloc(uint32 tag_id, usize size);

    /**
     * CPU 메모리 해제를 기록합니다.
     * @param tag_id 해제된 메모리의 태그 ID
     * @param size 해제된 크기 (바이트)
     */
    static void TrackFree(uint32 tag_id, usize size);

    /**
     * GPU 메모리 할당을 기록합니다.
     * @param tag_id 할당에 사용된 태그 ID
     * @param size 할당된 크기 (바이트)
     */
    static void TrackGpuAlloc(uint32 tag_id, usize size);

    /**
     * GPU 메모리 해제를 기록합니다.
     * @param tag_id 해제된 메모리의 태그 ID
     * @param size 해제된 크기 (바이트)
     */
    static void TrackGpuFree(uint32 tag_id, usize size);

    /**
     * 현재 등록된 모든 메모리 태그의 목록을 반환합니다.
     * @return MemoryTag 구조체의 ArrayView
     */
    [[nodiscard]] static ArrayView<MemoryTag> GetTags();

    /**
     * 전체 CPU 메모리 할당량을 반환합니다.
     * @return 총 할당된 CPU 메모리 (바이트)
     */
    [[nodiscard]] static usize GetTotalCpuAllocated();

    /**
     * 전체 GPU 메모리 할당량을 반환합니다.
     * @return 총 할당된 GPU 메모리 (바이트)
     */
    [[nodiscard]] static usize GetTotalGpuAllocated();

private:
    // 최대 등록 가능한 태그 개수
    static constexpr uint32 MAX_MEMORY_TAGS = 256;

    inline static TracyLockable(std::mutex, registry_mutex);

    static std::atomic<uint32> registered_count;
    static FixedArray<MemoryTag, MAX_MEMORY_TAGS> tags;
    static HashMap<StringName, uint32> tag_lookup;

    static std::atomic<usize> total_cpu_allocated;
    static std::atomic<usize> total_gpu_allocated;
#else
    FORCE_INLINE static uint32 GetOrRegisterTag([[maybe_unused]] const StringName& name) { return 0; }

    FORCE_INLINE static uint32 SetCurrentTag([[maybe_unused]] uint32 tag_id) { return 0; }
    FORCE_INLINE static uint32 GetCurrentTag() { return 0; }

    FORCE_INLINE static void TrackAlloc([[maybe_unused]] uint32 tag_id, [[maybe_unused]] usize size) {}
    FORCE_INLINE static void TrackFree([[maybe_unused]] uint32 tag_id, [[maybe_unused]] usize size) {}

    FORCE_INLINE static void TrackGpuAlloc([[maybe_unused]] uint32 tag_id, [[maybe_unused]] usize size) {}
    FORCE_INLINE static void TrackGpuFree([[maybe_unused]] uint32 tag_id, [[maybe_unused]] usize size) {}

    FORCE_INLINE static ArrayView<MemoryTag> GetTags() { return {}; }
    FORCE_INLINE static usize GetTotalCpuAllocated() { return 0; }
    FORCE_INLINE static usize GetTotalGpuAllocated() { return 0; }
#endif
};

/**
 * RAII 패턴을 사용하여 현재 블록(Scope)의 메모리 태그를 변경하고, 블록을 벗어날 때 이전 태그로 복구합니다.
 */
class ScopedMemoryTag
{
public:
#if SE_ENABLE_MEMORY_TRACKING
    explicit ScopedMemoryTag(const StringName& name)
    {
        const uint32 new_id = MemoryStats::GetOrRegisterTag(name);
        prev_id = MemoryStats::SetCurrentTag(new_id);
    }

    explicit ScopedMemoryTag(uint32 id)
    {
        prev_id = MemoryStats::SetCurrentTag(id);
    }

    ~ScopedMemoryTag()
    {
        MemoryStats::SetCurrentTag(prev_id);
    }

private:
    uint32 prev_id = 0;

#else

    explicit ScopedMemoryTag([[maybe_unused]] const StringName& name) {}
    explicit ScopedMemoryTag([[maybe_unused]] uint32 id) {}
    ~ScopedMemoryTag() = default;
#endif

public:
    // 복사 및 이동 금지
    ScopedMemoryTag(const ScopedMemoryTag&) = delete;
    ScopedMemoryTag& operator=(const ScopedMemoryTag&) = delete;
    ScopedMemoryTag(ScopedMemoryTag&&) noexcept = delete;
    ScopedMemoryTag& operator=(ScopedMemoryTag&&) noexcept = delete;
};
}  // namespace se

/**
 * 현재 스코프의 메모리 태그를 설정합니다.
 * @param name 태그 이름 (문자열 리터럴 또는 StringName)
 */
#define SE_MEM_SCOPE(name) \
    se::ScopedMemoryTag SE_UNIQUE_TOKEN(_se_mem_tag_){ name }

/**
 * 미리 캐싱된 ID를 사용하여 현재 스코프의 메모리 태그를 설정합니다.
 * @param id 태그 ID (uint32)
 */
#define SE_MEM_SCOPE_ID(id) \
    se::ScopedMemoryTag SE_UNIQUE_TOKEN(_se_mem_tag_){ id }
