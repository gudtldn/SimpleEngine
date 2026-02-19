#pragma once

#include <filesystem>

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/ArrayView.h"
#include "SimpleEngine/Core/Container/Optional.h"
#include "SimpleEngine/Core/Container/StringView.h"
#include "SimpleEngine/Core/Error/Expected.h"
#include "SimpleEngine/Core/Error/IError.h"
#include "SimpleEngine/Core/Functional/Function.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Core/Types/Path.h"


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
        UnexpectedEOF,    // 끝까지 읽지 못함, 예기치 않은 EOF
        OutOfMemory,      // 메모리 부족
        UnknownError      // 알 수 없는 에러
    };

    static FileReadError NotFound(String&& msg)   { return { EType::FileNotFound,     std::move(msg) }; }
    static FileReadError OpenFailed(String&& msg) { return { EType::FileOpenFailed,   std::move(msg) }; }
    static FileReadError Permission(String&& msg) { return { EType::PermissionDenied, std::move(msg) }; }
    static FileReadError Format(String&& msg)     { return { EType::InvalidFormat,    std::move(msg) }; }
    static FileReadError Read(String&& msg)       { return { EType::ReadFailed,       std::move(msg) }; }
    static FileReadError Write(String&& msg)      { return { EType::WriteFailed,      std::move(msg) }; }
    static FileReadError EndOfFile(String&& msg)  { return { EType::UnexpectedEOF,    std::move(msg) }; }
    static FileReadError OutOfMem(String&& msg)   { return { EType::OutOfMemory,      std::move(msg) }; }
    static FileReadError Unknown(String&& msg)    { return { EType::UnknownError,     std::move(msg) }; }

    [[nodiscard]] virtual const char* What() const noexcept override { return message.CStr(); }
    [[nodiscard]] virtual const IError* Source() const noexcept override { return nullptr; }

    [[nodiscard]] EType GetType() const noexcept { return type; }

private:
    FileReadError(EType in_type, String&& msg)
        : type(in_type), message(std::move(msg)) {}

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
public:
    DirectoryEntry() = default;
    explicit DirectoryEntry(std::filesystem::directory_entry entry);

    /** 엔트리의 경로를 반환합니다. */
    [[nodiscard]] Path GetPath() const;

    /** 디렉토리인지 확인합니다. */
    [[nodiscard]] bool IsDirectory() const;

    /** 일반 파일인지 확인합니다. */
    [[nodiscard]] bool IsFile() const;

    /** 심볼릭 링크인지 확인합니다. */
    [[nodiscard]] bool IsSymlink() const;

    /** 파일 크기를 반환합니다. (파일이 아닌 경우 0) */
    [[nodiscard]] usize FileSize() const;

private:
    std::filesystem::directory_entry internal_entry;
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
    DirectoryIterator operator++(int);

    [[nodiscard]] reference operator*() const { return current_entry; }
    [[nodiscard]] pointer operator->() const { return &current_entry; }

    [[nodiscard]] bool operator==(const DirectoryIterator& other) const;
    [[nodiscard]] bool operator!=(const DirectoryIterator& other) const { return !(*this == other); }

    [[nodiscard]] DirectoryIterator begin() const { return *this; }
    [[nodiscard]] DirectoryIterator end() const { return {}; }

private:
    std::filesystem::directory_iterator internal_iter;
    DirectoryEntry current_entry;
    bool is_end = true;
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


    // =========================================================================
    // File Read/Write (Rust-style)
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
     * 파일을 고정된 크기의 청크 단위로 읽어 Callback으로 전달합니다.
     * @param path 파일 경로
     * @param chunk_size 한 번에 읽을 데이터의 최대 크기 (Bytes)
     * @param callback 읽은 데이터를 처리할 Callback 함수. false를 반환하면 읽기가 즉시 중단됩니다.
     */
    static FileResult<void> ReadChunked(
        const Path& path,
        usize chunk_size,
        const Function<bool(ArrayView<const uint8>)>& callback
    );

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
