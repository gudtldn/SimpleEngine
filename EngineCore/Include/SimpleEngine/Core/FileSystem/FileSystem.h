#pragma once

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/ArrayView.h"
#include "SimpleEngine/Core/Container/Optional.h"
#include "SimpleEngine/Core/Container/StringView.h"
#include "SimpleEngine/Core/Error/Expected.h"
#include "SimpleEngine/Core/Error/IError.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Core/Types/Path.h"

#include <generator>


namespace se
{
// =============================================================================
// FileReadError / FileResult
// =============================================================================

class FileReadError : public IError
{
public:
    enum class EType
    {
        FileNotFound,     // 파일이 존재하지 않음
        FileOpenFailed,   // 파일 열기 실패
        PermissionDenied, // 권한 문제
        InvalidFormat,    // 파일/데이터 포맷 이상
        ReadFailed,       // 읽기 실패
        WriteFailed,      // 쓰기 실패
        EndOfFile,        // 끝까지 읽지 못함, 예기치 않은 EOF
        OutOfMemory,      // 메모리 부족
        UnknownError      // 알 수 없는 에러
    };
    using enum EType;

public:
    FileReadError(EType in_type, String&& msg)
        : type(in_type), message(std::move(msg)) {}

    [[nodiscard]] virtual const char* What() const noexcept override { return message.CStr(); }
    [[nodiscard]] virtual const IError* Source() const noexcept override { return nullptr; }

    [[nodiscard]] EType GetType() const noexcept { return type; }

private:
    EType type;
    String message;
};

template <typename T>
using FileResult = Expected<T, FileReadError>;


// =============================================================================
// DirectoryEntry / DirectoryIterator
// =============================================================================

/**
 * 파일 또는 디렉토리의 정보를 담고 있는 구조체
 */
class SE_CORE_API DirectoryEntry
{
    friend class DirectoryIterator;

public:
    DirectoryEntry() = default;

    /** 엔트리의 경로를 반환합니다. */
    [[nodiscard]] const Path& GetPath() const;

    /** 디렉토리인지 확인합니다. */
    [[nodiscard]] bool IsDirectory() const;

    /** 일반 파일인지 확인합니다. */
    [[nodiscard]] bool IsFile() const;

    /** 심볼릭 링크인지 확인합니다. (SDL3 한계로 항상 false 반환) */
    [[nodiscard]] bool IsSymlink() const;

    /** 파일 크기를 반환합니다. (파일이 아닌 경우 0) */
    [[nodiscard]] usize FileSize() const;

    /** 마지막 수정 시간을 반환합니다. (SDL3 시간: Unix epoch 초 단위) */
    [[nodiscard]] uint64 LastWriteTime() const;

private:
    Path entry_path;
    bool is_directory = false;
    bool is_file = false;
    usize file_size = 0;
    uint64 last_write_time = 0;
};


/**
 * 특정 경로 내의 디렉터리 항목들을 순회하는 Iterator
 */
class SE_CORE_API DirectoryIterator
{
public:
    using iterator_category = std::input_iterator_tag;
    using value_type = DirectoryEntry;
    using difference_type = std::ptrdiff_t;
    using pointer = const DirectoryEntry*;
    using reference = const DirectoryEntry&;

    DirectoryIterator() = default;
    explicit DirectoryIterator(const Path& path);

    DirectoryIterator& operator++();
    void operator++(int);

    [[nodiscard]] reference operator*() const { return entries[current_index]; }
    [[nodiscard]] pointer operator->() const { return &entries[current_index]; }

    [[nodiscard]] bool operator==(const DirectoryIterator& other) const;
    [[nodiscard]] bool operator!=(const DirectoryIterator& other) const { return !(*this == other); }

    [[nodiscard]] DirectoryIterator begin() const { return *this; }
    [[nodiscard]] DirectoryIterator end() const { return {}; }

private:
    [[nodiscard]] bool IsEnd() const { return current_index >= entries.Len(); }

    Array<DirectoryEntry> entries;
    usize current_index = 0;
};


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
     * @note SDL3에는 심볼릭 링크 해소 API가 없으므로 Absolute + 존재 확인만 수행합니다.
     * @param path 변환할 경로 (반드시 존재해야 함)
     * @return 정규화된 절대 경로. 실패 시 nullopt를 반환합니다.
     */
    [[nodiscard]] static Optional<Path> Canonical(const Path& path);


    // =========================================================================
    // Directory Operations
    // =========================================================================

    /**
     * 디렉토리를 생성합니다. 중간 디렉토리가 없으면 함께 생성합니다.
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
     * 파일 또는 디렉토리가 존재하는지 확인합니다.
     * @param path 확인할 경로
     * @return 존재하면 true, 존자하지 않으면 false
     */
    [[nodiscard]] static bool Exists(const Path& path);

    /**
     * 파일의 크기를 바이트 단위로 반환합니다.
     * @param path 파일 경로
     * @return 파일 크기. 실패 시 nullopt
     */
    [[nodiscard]] static Optional<usize> FileSize(const Path& path);

    /**
     * 파일 또는 디렉토리의 마지막 수정 시간을 반환합니다. (SDL3: Unix epoch 초 단위)
     * @param path 대상 경로
     * @return 마지막 수정 시간. (uint64) 실패 시 nullopt
     */
    [[nodiscard]] static Optional<uint64> LastWriteTime(const Path& path);

    // =========================================================================
    // File Read/Write
    // =========================================================================

    /**
     * 파일 전체 내용을 문자열로 읽습니다.
     * @param path 파일 경로
     * @return 파일 내용. 실패 시 FileReadError
     */
    [[nodiscard]] static FileResult<String> ReadToString(const Path& path);

    /**
     * 파일 전체 내용을 바이트 배열로 읽습니다.
     * @param path 파일 경로
     * @return 파일 내용. 실패 시 FileReadError
     */
    [[nodiscard]] static FileResult<Array<uint8>> ReadBytes(const Path& path);

    /**
     * 파일을 고정된 크기의 청크 단위로 읽어 Generator로 반환합니다.
     * @param path 파일 경로
     * @param chunk_size 한 번에 읽을 데이터의 최대 크기 (Bytes)
     * @return 읽은 데이터를 처리할 Generator
     */
    static std::generator<FileResult<ArrayView<const uint8>>>ReadChunked(Path path, usize chunk_size);

    /**
     * 문자열을 파일에 씁니다. (기존 내용 덮어쓰기)
     * @param path 파일 경로
     * @param content 쓸 내용
     * @return 성공 시 true
     */
    static bool WriteString(const Path& path, StringView content);

    /**
     * 바이트 배열을 파일에 씁니다. (기존 내용 덮어쓰기)
     * @param path 파일 경로
     * @param data 쓸 데이터
     * @return 성공 시 true
     */
    static bool Write(const Path& path, ArrayView<const uint8> data);


    // =========================================================================
    // Directory Iteration
    // =========================================================================

    /**
     * 디렉토리 내 엔트리를 순회합니다.
     * @param path 디렉토리 경로
     */
    [[nodiscard]] static DirectoryIterator ReadDir(const Path& path);
};
}  // namespace se
