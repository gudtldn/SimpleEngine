module SE.Types;
import :VirtualPath;


VPath::VPath(std::u8string_view path)
{
    ParseAndNormalize(path);
}

VPath::VPath(const std::filesystem::path& path)
{
    ParseAndNormalize(path.generic_u8string());
}

VPath VPath::operator/(std::u8string_view relative_path) const
{
    if (relative_path.empty())
    {
        return *this;
    }

    se::u8string new_path_str = full_path;

    // 슬래시 중복 방지
    if (new_path_str.back() != u8'/' && relative_path.front() != u8'/')
    {
        new_path_str += u8'/';
    }
    new_path_str += relative_path;

    return { new_path_str };
}

std::u8string_view VPath::GetScheme() const noexcept
{
    if (HasScheme())
    {
        return std::u8string_view(full_path.data(), scheme_len);
    }
    return {};
}

std::u8string_view VPath::GetPathPart() const noexcept
{
    if (!IsValid())
    {
        return std::u8string_view(full_path.data() + path_offset, full_path.length() - path_offset);
    }
    return {};
}

VPath VPath::GetParentPath() const
{
    if (!IsValid())
    {
        return {};
    }

    const auto last_slash = full_path.find_last_of(u8'/');

    // "Assets://foo.txt" -> "Assets://"
    // "Assets://bar/" -> "Assets://"
    if (last_slash <= path_offset)
    {
        return { std::u8string_view(full_path.data(), path_offset) };
    }

    return { std::u8string_view(full_path.data(), last_slash) };
}

std::u8string_view VPath::GetFilename() const noexcept
{
    if (!IsValid())
    {
        return {};
    }

    const auto last_slash = full_path.find_last_of(u8'/');
    if (last_slash == se::u8string::npos)
    {
        return GetPathPart(); // 스키마는 없고 파일명만 있는 경우
    }
    return std::u8string_view(full_path.data() + last_slash + 1);
}

std::u8string_view VPath::GetExtension() const noexcept
{
    const std::u8string_view filename = GetFilename();
    if (filename.empty())
    {
        return {};
    }

    const auto last_dot = filename.find_last_of(u8'.');
    if (last_dot == std::u8string_view::npos)
    {
        return {}; // 확장자 없음
    }
    return filename.substr(last_dot);
}

std::u8string_view VPath::GetStem() const noexcept
{
    const std::u8string_view filename = GetFilename();
    if (filename.empty())
    {
        return {};
    }

    const auto last_dot = filename.find_last_of(u8'.');
    if (last_dot == std::u8string_view::npos)
    {
        return filename; // 확장자 없음
    }
    return filename.substr(0, last_dot);
}

void VPath::ParseAndNormalize(std::u8string_view path)
{
    if (path.empty())
    {
        return;
    }

    full_path.reserve(path.length());

    // 경로 정규화 (\ -> /) 및 복사
    for (const char8_t c : path)
    {
        full_path += (c == u8'\\') ? u8'/' : c;
    }

    // 스키마 파싱 ("scheme://")
    const auto scheme_separator = full_path.find(u8"://");
    if (scheme_separator != se::u8string::npos)
    {
        scheme_len = static_cast<uint16>(scheme_separator);
        path_offset = static_cast<uint16>(scheme_separator + 3); // "://" 길이만큼 건너뜀
    }
    else
    {
        // 스키마가 없는 경우 (상대 경로)
        scheme_len = 0;
        path_offset = 0;
    }
}
