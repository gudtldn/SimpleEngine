#pragma once

#include "SimpleEngine/Core/Container/Optional.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Core/Types/Path.h"


namespace se
{
/**
 * 파일시스템 I/O 작업을 위한 유틸리티
 */
struct SE_CORE_API FileSystem
{
    FileSystem() = delete;

    // =========================================================================
    // Path Operations
    // =========================================================================

    /**
     * 경로를 절대 경로로 변환합니다.
     * @param path 변환할 경로
     * @return 절대 경로. 실패 시 빈 경로를 반환합니다.
     */
    [[nodiscard]] static Path Absolute(const Path& path);

    /**
     * 경로를 정규화된 절대 경로(canonical path)로 변환합니다.
     * 심볼릭 링크를 해석하고 `.`, `..`을 제거합니다.
     * @param path 변환할 경로 (반드시 존재해야 함)
     * @return 정규화된 절대 경로. 실패 시 nullopt를 반환합니다.
     */
    [[nodiscard]] static Optional<Path> Canonical(const Path& path);


    // =========================================================================
    // Directory Operations
    // =========================================================================

    /**
     * 디렉토리를 생성합니다.
     * @param path 생성할 디렉토리 경로
     * @return 성공 시 true, 실패 시 false (이미 존재하는 경우도 true)
     */
    static bool CreateDirectory(const Path& path);

    /**
     * 디렉토리를 재귀적으로 생성합니다.
     * @param path 생성할 디렉토리 경로
     * @return 성공 시 true, 실패 시 false (이미 존재하는 경우도 true)
     */
    static bool CreateDirectories(const Path& path);


    // =========================================================================
    // File Operations
    // =========================================================================

    /**
     * 파일 또는 디렉토리를 삭제합니다.
     * @param path 삭제할 경로
     * @return 삭제 성공 시 true, 실패하거나 존재하지 않으면 false
     */
    static bool Remove(const Path& path);

    /**
     * 파일 또는 디렉토리를 재귀적으로 삭제합니다.
     * @param path 삭제할 경로
     * @return 삭제된 항목 수
     */
    static usize RemoveAll(const Path& path);

    /**
     * 파일 또는 디렉토리를 복사합니다.
     * @param from 원본 경로
     * @param to 대상 경로
     * @return 성공 시 true
     */
    static bool Copy(const Path& from, const Path& to);

    /**
     * 파일 또는 디렉토리의 이름을 변경하거나 이동합니다.
     * @param from 원본 경로
     * @param to 대상 경로
     * @return 성공 시 true
     */
    static bool Rename(const Path& from, const Path& to);


    // =========================================================================
    // File Info
    // =========================================================================

    /**
     * 파일의 크기를 바이트 단위로 반환합니다.
     * @param path 파일 경로
     * @return 파일 크기. 실패 시 nullopt
     */
    [[nodiscard]] static Optional<usize> FileSize(const Path& path);
};
}  // namespace se
