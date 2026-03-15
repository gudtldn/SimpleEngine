#pragma once

#include "SimpleEngine/Core/Container/Optional.h"
#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/Container/StringView.h"

#include <compare>
#include <functional>


namespace se
{
/**
 * 파일 시스템 경로를 다루는 클래스
 *
 * 내부적으로 se::String으로 관리하며, 항상 정규화된 상태를 유지합니다.
 * - 구분자는 '/'로 통일
 * - 불필요한 "./" 제거, "A/B/../C"는 "A/C"로 압축
 * - 연속된 슬래시 "//" 제거, 후행 슬래시 제거
 *
 * @note 경로 비교(==, <=>)는 모든 플랫폼에서 case-sensitive로 동작합니다.
 *       크로스 플랫폼 예측 가능성을 위한 의도적 설계입니다.
 */
class SE_CORE_API Path
{
public:
    Path() = default;
    Path(const char* in_path);
    Path(const String& in_path);
    Path(StringView in_path);

    Path(const Path&) = default;
    Path& operator=(const Path&) = default;
    Path(Path&&) = default;
    Path& operator=(Path&&) = default;

public:
    // --- Modifiers ---

    /**
     * 현재 경로 뒤에 새로운 경로를 이어 붙입니다.
     * 예: "path/to" + "file" -> "path/to/file"
     */
    Path& Append(const Path& other);
    Path& operator/=(const Path& other);

    /**
     * 현재 경로 문자열 뒤에 문자열을 그대로 붙입니다.
     * 예: "file" + ".txt" -> "file.txt"
     */
    Path& Concat(const String& str);
    Path& operator+=(const String& str);

    /**
     * 파일명을 변경합니다.
     * 예: "dir/old.txt" -> "dir/new.png"
     */
    Path& SetFileName(const String& name);

    /**
     * 확장자를 변경합니다.
     * 예: "file.txt" -> "file.jpg"
     * @note 점(.)이 포함되지 않은 경우 자동으로 추가됩니다.
     */
    Path& SetExtension(const String& extension);

public:
    // --- Producers ---

    /** 두 경로를 결합하여 새로운 Path를 반환합니다. */
    [[nodiscard]] Path operator/(const Path& other) const;

    /** 파일명이 변경된 새로운 Path를 반환합니다. */
    [[nodiscard]] Path WithFileName(const String& name) const;

    /** 확장자가 변경된 새로운 Path를 반환합니다. */
    [[nodiscard]] Path WithExtension(const String& extension) const;

    /**
     * base 경로를 기준으로 한 상대 경로를 계산하여 반환합니다.
     * 예: (*this)"/A/B/C", base="/A" -> "B/C"
     * @note 절대/상대 불일치, 루트 접두사 불일치, 공통 접두사가 없는 경우 nullopt를 반환합니다.
     *       (예: "/A/B"와 "/C/D"처럼 공통 경로가 없는 경우)
     */
    [[nodiscard]] Optional<Path> RelativeTo(const Path& base) const;

public:
    // --- Components ---

    /**
     * 부모 경로를 반환합니다.
     * 부모 경로가 없는 경우(루트이거나 비어있음) nullopt를 반환합니다.
     */
    [[nodiscard]] Optional<Path> Parent() const;

    /** 파일명(확장자 포함)을 반환합니다. */
    [[nodiscard]] Optional<String> FileName() const;

    /** 확장자를 제외한 파일명(Stem)을 반환합니다. */
    [[nodiscard]] Optional<String> FileStem() const;

    /** 확장자(`.` 포함)를 반환합니다. */
    [[nodiscard]] Optional<String> Extension() const;

public:
    // --- Queries ---

    /** 경로가 비어있는지 확인합니다. */
    [[nodiscard]] bool IsEmpty() const;

    /** 절대 경로인지 확인합니다. (예: "C:/", "/") */
    [[nodiscard]] bool IsAbsolute() const;

    /** 상대 경로인지 확인합니다. */
    [[nodiscard]] bool IsRelative() const;

    /**
     * 이 경로가 base 경로의 하위에 속해있는지 확인합니다.
     * 예: "/A/B"는 "/A"의 서브 경로입니다.
     */
    [[nodiscard]] bool IsSubPathOf(const Path& base) const;

    // --- Filesystem Queries ---

    /** 해당 경로에 파일이나 디렉토리가 실제로 존재하는지 확인합니다. */
    [[nodiscard]] bool Exists() const;

    /** 해당 경로가 디렉토리인지 확인합니다. */
    [[nodiscard]] bool IsDirectory() const;

    /** 해당 경로가 일반 파일인지 확인합니다. */
    [[nodiscard]] bool IsFile() const;

    // --- Conversions ---

    /** 경로를 UTF-8 문자열로 반환합니다. (내부 문자열의 const 참조) */
    [[nodiscard]] const String& ToString() const;

    /** 경로를 C 문자열(null-terminated)로 반환합니다. */
    [[nodiscard]] const char* CStr() const;

    void Swap(Path& other) noexcept;

public:
    // --- Operators ---

    [[nodiscard]] bool operator==(const Path& other) const;
    [[nodiscard]] std::strong_ordering operator<=>(const Path& other) const;

    friend void swap(Path& lhs, Path& rhs) noexcept { lhs.Swap(rhs); }

private:
    /** 입력 문자열을 정규화하여 반환합니다. O(n) 단일 패스. */
    static String NormalizePath(StringView input);

    /** 경로 문자열에서 루트 접두사 길이를 반환합니다. (예: "/" -> 1, "C:/" -> 3) */
    static usize DetectRootLength(StringView view);

    friend struct std::hash<Path>;
    String path;
};
} // namespace se

template <>
struct std::hash<se::Path>
{
    size_t operator()(const se::Path& in_path) const noexcept
    {
        return std::hash<se::String>{}(in_path.path);
    }
};

template <>
struct std::formatter<se::Path> : std::formatter<se::String>
{
    auto format(const se::Path& path, std::format_context& ctx) const
    {
        return std::formatter<se::String>::format(path.ToString(), ctx);
    }
};
