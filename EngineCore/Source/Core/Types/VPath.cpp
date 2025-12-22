#include "SimpleEngine/Core/Types/VPath.h"


namespace se
{
VPath::VPath(const char* path)
    : VPath(std::string_view{ path })
{
}

VPath::VPath(const se::String& path)
    : VPath(std::string_view{ path })
{
}

VPath::VPath(std::string_view path)
{
    ParseAndNormalize(path);
}

VPath VPath::operator/(std::string_view relative_path) const
{
    if (relative_path.empty())
    {
        return *this;
    }

    VPath new_path;
    new_path.full_path = full_path;

    // 슬래시 중복 방지
    se::String& new_path_str = new_path.full_path;
    const std::string_view view{ new_path_str };
    if (view.back() != '/' && relative_path.front() != '/')
    {
        new_path_str += '/';
    }
    new_path_str += relative_path;

    return new_path;
}

std::string_view VPath::GetScheme() const noexcept
{
    if (HasScheme())
    {
        return std::string_view{ full_path.Data(), scheme_len };
    }
    return {};
}

std::string_view VPath::GetPathPart() const noexcept
{
    if (IsValid())
    {
        return std::string_view{ full_path.Data() + path_offset, full_path.ByteLen() - path_offset };
    }
    return {};
}

VPath VPath::GetParentPath() const
{
    if (IsValid())
    {
        if (const Optional last_slash_opt = full_path.FindLast("/"))
        {
            // "Assets://foo.txt" -> "Assets://"
            // "Assets://bar/" -> "Assets://"
            if (*last_slash_opt <= path_offset)
            {
                return { std::string_view{ full_path.Data(), path_offset } };
            }

            return { std::string_view{ full_path.Data(), *last_slash_opt } };
        }
    }
    return {};
}

std::string_view VPath::GetFilename() const noexcept
{
    if (IsValid())
    {
        const Optional last_slash_opt = full_path.FindLast("/");
        if (!last_slash_opt.HasValue())
        {
            return GetPathPart(); // 스키마는 없고 파일명만 있는 경우
        }
        return std::string_view{ full_path.Data() + *last_slash_opt + 1 };
    }
    return {};
}

std::string_view VPath::GetExtension() const noexcept
{
    const std::string_view filename = GetFilename();
    if (filename.empty())
    {
        return {};
    }

    const auto last_dot = filename.find_last_of('.');
    if (last_dot == std::string_view::npos)
    {
        return {}; // 확장자 없음
    }
    return filename.substr(last_dot);
}

std::string_view VPath::GetStem() const noexcept
{
    const std::string_view filename = GetFilename();
    if (filename.empty())
    {
        return {};
    }

    const auto last_dot = filename.find_last_of('.');
    if (last_dot == std::string_view::npos)
    {
        return filename; // 확장자 없음
    }
    return filename.substr(0, last_dot);
}

void VPath::ParseAndNormalize(std::string_view path)
{
    if (path.empty())
    {
        return;
    }

    full_path.ResizeForOverwrite(path.length());

    // 경로 정규화 (\ -> /) 및 복사
    char* dest = full_path.Data();
    for (size_t i = 0; i < path.length(); ++i)
    {
        const char c = path[i];
        dest[i] = (c == '\\') ? '/' : c;
    }

    // 스키마 파싱 ("scheme://")
    if (const Optional scheme_separator_opt = full_path.Find("://"))
    {
        scheme_len = static_cast<uint16>(*scheme_separator_opt);
        path_offset = static_cast<uint16>(*scheme_separator_opt + 3); // "://" 길이만큼 건너뜀
    }
    else
    {
        // 스키마가 없는 경우 (상대 경로)
        scheme_len = 0;
        path_offset = 0;
    }
}
}  // namespace se
