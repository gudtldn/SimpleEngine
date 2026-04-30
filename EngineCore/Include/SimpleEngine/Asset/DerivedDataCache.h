#pragma once

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/ArrayView.h"
#include "SimpleEngine/Core/Types/Guid.h"
#include "SimpleEngine/Core/Types/HashDigest.h"
#include "SimpleEngine/Core/Types/Path.h"


namespace se
{
/**
 * DDC 캐시 파일에서 읽어온 엔트리 정보
 *
 * 헤더에 기록된 source_hash, cache_version과 함께
 * 실제 직렬화된 Asset 데이터(payload)를 담습니다.
 */
struct CacheEntry
{
    /** 소스 파일의 해시 */
    ContentHash source_hash;

    /** 캐시 스키마 버전 (Importer 출력 포맷 변경 시 증가) */
    uint32 cache_version = 0;

    /** 직렬화할 Asset 바이너리 데이터 */
    Array<uint8> payload;
};

/**
 * Derived Data Cache (DDC)
 *
 * Import 파이프라인이 생성한 Asset 바이너리를 디스크에 캐싱합니다.
 * 소스 파일이 변경되지 않은 경우 재임포트 없이 캐시에서 바로 로드할 수 있어, 에디터 시작 시간과 Asset 로드 시간을 크게 줄입니다.
 *
 * 디렉토리 구조:
 *   DDC_ROOT/
 *     ab/                                          <- GUID 앞 2글자로 버킷
 *       abcdef01-2345-6789-abcd-ef0123456789.cache
 *
 * 캐시 파일 포맷 (MemoryArchive 바이너리 직렬화):
 *   [4  bytes]   Magic: "SEDC"
 *   [4  bytes]   Format Version: uint32
 *   [8+N bytes]  Source Hash: String (length-prefixed)
 *   [4  bytes]   Cache Schema Version: uint32
 *   [8  bytes]   Payload Size: uint64
 *   [M  bytes]   Payload: BinaryBlob (raw 바이너리 데이터)
 *
 * Atomic Write를 사용하여 쓰기 도중 크래시로 인한 corruption을 방지합니다.
 */
class SE_CORE_API DerivedDataCache
{
public:
    explicit DerivedDataCache(Path in_root_path);
    ~DerivedDataCache() = default;

    // 복사 금지 & 이동만 허용
    DerivedDataCache(const DerivedDataCache&) = delete;
    DerivedDataCache& operator=(const DerivedDataCache&) = delete;
    DerivedDataCache(DerivedDataCache&&) = default;
    DerivedDataCache& operator=(DerivedDataCache&&) = default;

public:
    /**
     * 메모리 버퍼로부터 캐시 엔트리를 파싱합니다.
     * AsyncFileIO로 읽은 버퍼를 Load() 없이 직접 파싱할 때 사용합니다.
     * @param buffer_view 캐시 파일의 전체 바이너리 데이터
     * @return 파싱된 CacheEntry. 포맷이 유효하지 않으면 nullopt
     */
    [[nodiscard]] static Optional<CacheEntry> ParseFromBuffer(ArrayView<const uint8> buffer_view);

public:
    /**
     * Asset 바이너리를 캐시에 저장합니다.
     * @param guid 캐시 키 (Asset의 GUID)
     * @param entry 캐시에 저장할 Entry
     * @return 저장 성공 여부
     */
    bool Store(
        const Guid& guid,
        CacheEntry&& entry
    );

    /**
     * 캐시에서 Asset 바이너리를 읽어옵니다.
     * @param guid 읽어올 Asset의 GUID
     * @return 캐시 엔트리. 파일이 없거나 손상된 경우 nullopt
     */
    [[nodiscard]] Optional<CacheEntry> Load(const Guid& guid) const;

    /**
     * 캐시가 존재하고 유효한지 확인합니다.
     * @param guid 확인할 Asset의 GUID
     * @param source_hash 기대하는 소스 해시
     * @param cache_version 기대하는 캐시 스키마 버전
     * @return 캐시가 유효한 경우 true
     */
    [[nodiscard]] bool IsValid(
        const Guid& guid,
        const ContentHash& source_hash,
        uint32 cache_version
    ) const;

    /**
     * 해당 GUID의 캐시 파일이 존재하는지 확인합니다.
     * @note 파일 내용의 유효성은 검사하지 않습니다.
     * @param guid 확인할 Asset의 GUID
     * @return 캐시 파일이 존재하면 true
     */
    [[nodiscard]] bool Contains(const Guid& guid) const;

    /**
     * 특정 GUID의 캐시 파일을 삭제합니다.
     * @param guid 삭제할 Asset의 GUID
     * @return 삭제 성공 여부 (파일이 없으면 true 반환)
     */
    bool Remove(const Guid& guid);

    /** DDC 루트 디렉토리 내의 모든 캐시를 삭제합니다. */
    void Clear();

    /** DDC 루트 경로를 반환합니다. */
    [[nodiscard]] FORCE_INLINE const Path& GetRootPath() const { return root_path; }

    /** GUID로부터 캐시 파일의 전체 경로를 계산합니다. (버킷 디렉토리 포함) */
    [[nodiscard]] Path BuildCachePath(const Guid& guid) const;

private:
    /** GUID로부터 임시 파일의 전체 경로를 계산합니다. (atomic write용) */
    [[nodiscard]] Path BuildTempPath(const Guid& guid) const;

private:
    Path root_path;
};
} // namespace se
