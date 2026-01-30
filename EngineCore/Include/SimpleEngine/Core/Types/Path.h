#pragma once

#include <filesystem>
#include <compare>
#include <functional>

#include "SimpleEngine/Core/Container/Optional.h"
#include "SimpleEngine/Core/Container/String.h"


namespace se
{
/**
 * 파일 시스템 경로를 다루는 클래스
 */
class SE_CORE_API Path
{
public:
    Path() = default;
    Path(const char* in_path);
    Path(const String& in_path);
    Path(std::string_view in_path);
    Path(std::filesystem::path in_path);

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
     * 경로를 정규화합니다.
     * ".." (상위 폴더), "." (현재 폴더) 등의 참조를 정리하여 최단 경로로 만듭니다.
     * 예: "A/./B/../C" -> "A/C"
     */
    Path& Normalize();

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

    /** 정규화된 새로운 Path를 반환합니다. */
    [[nodiscard]] Path GetNormalized() const;

    /**
     * base 경로를 기준으로 한 상대 경로를 계산하여 반환합니다.
     * 예: (*this)"/A/B/C", base="/A" -> "B/C"
     * @note 경로가 base 내부에 있지 않거나 계산 불가능할 경우 nullopt를 반환할 수 있습니다.
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

    /** 경로를 UTF-8 문자열로 변환하여 반환합니다. */
    [[nodiscard]] String ToString() const;

    // [[nodiscard]] const char* CStr() const; // 필요 시 주석 해제

    void Swap(Path& other) noexcept;

public:
    // --- Operators ---

    /** 내부 std::filesystem::path 객체를 반환합니다. (나중에 API 변경 가능성 있음) */
    [[nodiscard]] operator const std::filesystem::path&() const { return internal_path; }

    [[nodiscard]] bool operator==(const Path& other) const;
    [[nodiscard]] std::strong_ordering operator<=>(const Path& other) const;

    friend void swap(Path& lhs, Path& rhs) noexcept { lhs.Swap(rhs); }

private:
    friend struct std::hash<Path>;
    std::filesystem::path internal_path;
};
} // namespace se

template <>
struct std::hash<se::Path>
{
    size_t operator()(const se::Path& in_path) const noexcept
    {
        return std::filesystem::hash_value(in_path.internal_path);
    }
};

template <>
struct std::formatter<se::Path> : std::formatter<se::String>
{
    auto format(const se::Path& path, std::format_context& ctx) const
    {
        const se::String str = path.ToString();
        return std::formatter<se::String>::format(str, ctx);
    }
};
