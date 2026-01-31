#pragma once
#include <memory>
#include <shared_mutex>

#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Container/StringView.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Core/Types/StringName.h"

#include "tracy/Tracy.hpp"


namespace se
{
/**
 * StringPool 내부에서 관리되는 개별 문자열 Entry
 */
struct StringNameEntry
{
    /** StringStorage에 저장된 실제 문자열 포인터 (Null-terminated) */
    const char* display_name;

    /** 대소문자 무시(ASCII-only) 해시값 */
    uint64 comparison_hash;

    /** 문자열 길이 (Null 문자 제외) */
    uint32 length;
};

/**
 * 문자열 데이터를 연속된 메모리 블록(Chunk)에 저장하는 할당자
 * @todo 나중에 범용 LinearAllocator가 구현되면 교체하여 코드 중복 제거하기
 */
class StringStorage
{
public:
    static constexpr usize BLOCK_SIZE = 64ULL * 1024; // 64KB

public:
    StringStorage();
    ~StringStorage();

    /**
     * 문자열을 내부 블록에 복사하여 저장합니다.
     * @param view 저장할 문자열 View
     * @return 저장된 문자열의 시작 주소 (Null-terminated 보장)
     */
    const char* Store(StringView view);

private:
    /** 새 메모리 블록을 할당하고 current_block_ptr를 갱신합니다. */
    void AllocateNewBlock();

private:
    Array<std::unique_ptr<char[]>> blocks;
    char* current_block_ptr = nullptr;
    usize current_offset = 0;
};

/**
 * 전역 문자열 풀(String Interning)을 관리하는 싱글톤 클래스
 */
class StringNamePool
{
private:
    StringNamePool() = default;

public:
    ~StringNamePool() = default;

    // 이동 & 복사 생성자 제거
    StringNamePool(const StringNamePool&) = delete;
    StringNamePool& operator=(const StringNamePool&) = delete;
    StringNamePool(StringNamePool&&) = delete;
    StringNamePool& operator=(StringNamePool&&) = delete;

public:
    [[nodiscard]] static StringNamePool& Get();

    /** 문자열을 Pool에서 찾습니다. 없으면 nullopt */
    [[nodiscard]] Optional<const StringNameEntry&> Find(StringView view) const;

    /** 문자열을 Pool에서 찾고, 없으면 새로 만듭니다. */
    [[nodiscard]] const StringNameEntry& FindOrEmplace(StringView view);

private:
    mutable TracySharedLockable(std::shared_mutex, string_pool_mutex);

    StringStorage string_storage;

    // Key: Display Hash
    HashMap<uint64, StringNameEntry> entry_pool;

    // Key: Comparison Hash
    HashMap<uint64, const StringNameEntry*> lookup_map;
};
}  // namespace se
