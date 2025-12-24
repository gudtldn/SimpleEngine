#pragma once

#include <atomic>
#include <mutex>

#include "SimpleEngine/Core/Container/FixedArray.h"
#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Core/Memory/MemoryConfig.h"
#include "SimpleEngine/Core/Types/StringName.h"
#include "SimpleEngine/Utility/Common.h"

#include "tracy/Tracy.hpp"


namespace se::core
{
/**
 * @todo docs
 */
struct MemoryTag
{
    StringName name;
    std::atomic<usize> cpu_allocated = 0;
    std::atomic<usize> gpu_allocated = 0;
};

/**
 * @todo docs
 */
class SE_CORE_API MemoryStats
{
public:
#if SE_ENABLE_MEMORY_TRACKING
    /** 이름으로 태그를 등록하고 ID를 반환합니다. (중복 시 기존 ID 반환) */
    [[nodiscard]] static uint32 GetOrRegisterTag(const StringName& name);

    static uint32 SetCurrentTag(uint32 tag_id);
    static uint32 GetCurrentTag();

    // CPU 메모리 추적
    static void TrackAlloc(uint32 tag_id, usize size);
    static void TrackFree(uint32 tag_id, usize size);

    // GPU 메모리 추적
    static void TrackGpuAlloc(uint32 tag_id, usize size);
    static void TrackGpuFree(uint32 tag_id, usize size);

    [[nodiscard]] static std::span<MemoryTag> GetTags();
    [[nodiscard]] static usize GetTotalCpuAllocated();
    [[nodiscard]] static usize GetTotalGpuAllocated();

private:
    // 최대 태그 개수 제한
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

    FORCE_INLINE static std::span<MemoryTag> GetTags() { return {}; }
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
    ScopedMemoryTag(const ScopedMemoryTag&) = delete;
    ScopedMemoryTag& operator=(const ScopedMemoryTag&) = delete;
    ScopedMemoryTag(ScopedMemoryTag&&) noexcept = delete;
    ScopedMemoryTag& operator=(ScopedMemoryTag&&) noexcept = delete;
};
}  // namespace se::core

/** 현재 스코프의 메모리 태그를 설정합니다. */
#define SE_MEM_SCOPE(name) \
    se::core::ScopedMemoryTag SE_UNIQUE_TOKEN(_se_mem_tag_)(name)

/** 미리 캐싱된 ID를 사용하여 현재 스코프의 메모리 태그를 설정합니다. */
#define SE_MEM_SCOPE_ID(id) \
    se::core::ScopedMemoryTag SE_CONCAT_TOKEN(_se_mem_tag_)(id)
