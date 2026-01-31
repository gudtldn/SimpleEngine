#include "SimpleEngine/Core/Types/VPath.h"
#include "SimpleEngine/Core/Types/Path.h"
#include "SimpleEngine/Utility/VFS.h"


namespace se
{
VPath::VPath(const char* path)
    : VPath(StringView{ path })
{
}

VPath::VPath(const se::String& path)
    : VPath(StringView{ path })
{
}

VPath::VPath(StringView path)
{
    ParseAndNormalize(path);
}

VPath VPath::operator/(StringView relative_path) const
{
    if (relative_path.IsEmpty())
    {
        return *this;
    }

    VPath new_path;
    new_path.full_path = full_path;

    // 슬래시 중복 방지
    se::String& new_path_str = new_path.full_path;
    const StringView view{ new_path_str };
    if (view.Back() != '/' && relative_path.Front() != '/')
    {
        new_path_str += '/';
    }
    new_path_str += relative_path;

    return new_path;
}

StringView VPath::GetScheme() const noexcept
{
    if (HasScheme())
    {
        return StringView{ full_path.Data(), scheme_len };
    }
    return {};
}

StringView VPath::GetPathPart() const noexcept
{
    if (IsValid())
    {
        return StringView{ full_path.Data() + path_offset, full_path.ByteLen() - path_offset };
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
                return { StringView{ full_path.Data(), path_offset } };
            }

            return { StringView{ full_path.Data(), *last_slash_opt } };
        }
    }
    return {};
}

StringView VPath::GetFilename() const noexcept
{
    if (IsValid())
    {
        const Optional last_slash_opt = full_path.FindLast("/");
        if (!last_slash_opt.HasValue())
        {
            return GetPathPart(); // 스키마는 없고 파일명만 있는 경우
        }
        return StringView{ full_path.Data() + *last_slash_opt + 1 };
    }
    return {};
}

StringView VPath::GetExtension() const noexcept
{
    const StringView filename = GetFilename();
    if (filename.IsEmpty())
    {
        return {};
    }

    const auto last_dot = filename.FindLastOf('.');
    if (!last_dot.HasValue())
    {
        return {}; // 확장자 없음
    }
    return filename.Substr(*last_dot);
}

StringView VPath::GetStem() const noexcept
{
    const StringView filename = GetFilename();
    if (filename.IsEmpty())
    {
        return {};
    }

    const Optional last_dot = filename.FindLastOf('.');
    if (!last_dot.HasValue())
    {
        return filename; // 확장자 없음
    }
    return filename.Substr(0, *last_dot);
}

void VPath::ParseAndNormalize(StringView path)
{
    if (path.IsEmpty())
    {
        return;
    }

    full_path.ResizeForOverwrite(path.ByteLen());

    // 경로 정규화 (\ -> /) 및 복사
    char* dest = full_path.Data();
    for (usize i = 0; i < path.ByteLen(); ++i)
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

Path VPath::ToPath() const
{
    return VFS::Get().Resolve(*this, false).ValueOrDefault();
}

Optional<Path> VPath::Resolve() const
{
    return VFS::Get().Resolve(*this, true);
}

bool VPath::Exists() const
{
    return Resolve().HasValue();
}
}  // namespace se
