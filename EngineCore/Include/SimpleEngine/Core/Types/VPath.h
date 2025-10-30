#pragma once
#include <string_view>

#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Core/Types/StringName.h"


/**
 * 엔진의 파일 시스템을 추상화하는 가상 경로 타입
 */
class SE_CORE_API VPath
{
public:
    VPath() = default;
    ~VPath() = default;

    VPath(const char* path);
    VPath(const se::String& path);
    VPath(std::string_view path);

    VPath(const VPath&) = default;
    VPath& operator=(const VPath&) = default;
    VPath(VPath&&) noexcept = default;
    VPath& operator=(VPath&&) noexcept = default;

public:
    /** 현재 경로에 상대 경로를 추가하여 새로운 VPath 객체를 생성합니다 */
    [[nodiscard]] VPath operator/(std::string_view relative_path) const;

    // 비교 연산자
    [[nodiscard]] bool operator==(const VPath&) const = default;
    [[nodiscard]] auto operator<=>(const VPath&) const = default;

    /** 경로가 유효한지 (비어있지 않은지) 확인합니다. */
    [[nodiscard]] bool IsValid() const noexcept { return !full_path.IsEmpty(); }
    [[nodiscard]] explicit operator bool() const noexcept { return IsValid(); }

    /** 경로가 스키마("Assets://") 부분을 포함하는지 확인합니다. */
    [[nodiscard]] bool HasScheme() const noexcept { return scheme_len > 0; }

    /** 경로의 스킴 부분을 반환합니다. (예: "Assets") */
    [[nodiscard]] std::string_view GetScheme() const noexcept;

    /** 스킴을 제외한 순수 경로 부분을 반환합니다. (예: "/Textures/Player.png") */
    [[nodiscard]] std::string_view GetPathPart() const noexcept;

    /** 파일명을 제외한 부모 디렉토리 경로를 반환합니다. (예: "Assets://Textures") */
    [[nodiscard]] VPath GetParentPath() const;

    /** 확장자를 포함한 파일명을 반환합니다. (예: "Player.png") */
    [[nodiscard]] std::string_view GetFilename() const noexcept;

    /** 파일의 확장자를 반환합니다. (예: ".png") */
    [[nodiscard]] std::string_view GetExtension() const noexcept;

    /** 확장자를 제외한 파일명을 반환합니다. (예: "Player") */
    [[nodiscard]] std::string_view GetStem() const noexcept;

    /** 전체 경로를 se::String 참조로 반환합니다. */
    [[nodiscard]] const se::String& ToString() const noexcept { return full_path; }

    /** 전체 경로를 StringName으로 변환하여 반환합니다. */
    [[nodiscard]] StringName ToStringName() const { return StringName{ full_path }; }

private:
    /** 입력받은 경로 문자열을 파싱하고 정규화(\ -> /)하여 내부 멤버를 초기화합니다. */
    void ParseAndNormalize(std::string_view path);

private:
    se::String full_path;
    uint16 scheme_len = 0;  // 스키마의 길이, 0이면 스키마 없음
    uint16 path_offset = 0; // 경로 부분의 시작 인덱스
};

template <>
struct std::hash<VPath>
{
    size_t operator()(const VPath& path) const noexcept
    {
        return std::hash<se::String>{}(path.ToString());
    }
};

// VPath에 대한 std::formatter 특수화
template <>
struct std::formatter<VPath, char> : std::formatter<se::String>
{
    auto format(const VPath& path, std::format_context& ctx) const
    {
        return std::formatter<se::String>::format(path.ToString(), ctx);
    }
};
