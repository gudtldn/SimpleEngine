#include "Core/Types/Path.h"
#include "Core/Types/VPath.h"
#include "Utility/StringUtils.h"
#include "Utility/VFS.h"

#include <string>
#include <utility>


namespace se
{
Path::Path(const char* in_path)
{
    if (in_path)
    {
        internal_path = std::filesystem::path{ reinterpret_cast<const char8_t*>(in_path) };
    }
}

Path::Path(const String& in_path)
{
    if (!in_path.IsEmpty())
    {
        internal_path = std::filesystem::path{ reinterpret_cast<const char8_t*>(in_path.CStr()) };
    }
}

Path::Path(StringView in_path)
{
    if (!in_path.IsEmpty())
    {
        internal_path = std::filesystem::path{
            reinterpret_cast<const char8_t*>(in_path.Data()),
            reinterpret_cast<const char8_t*>(in_path.Data() + in_path.ByteLen())
        };
    }
}

Path::Path(std::filesystem::path in_path)
    : internal_path(std::move(in_path))
{
}

Path& Path::Append(const Path& other)
{
    internal_path /= other.internal_path;
    return *this;
}

Path& Path::operator/=(const Path& other)
{
    return Append(other);
}

Path& Path::Concat(const String& str)
{
    internal_path += std::filesystem::path{ reinterpret_cast<const char8_t*>(str.CStr()) };
    return *this;
}

Path& Path::operator+=(const String& str)
{
    return Concat(str);
}

Path& Path::Normalize()
{
    internal_path = internal_path.lexically_normal();
    return *this;
}

Path& Path::SetFileName(const String& name)
{
    internal_path.replace_filename({
        reinterpret_cast<const char8_t*>(name.CStr())
    });
    return *this;
}

Path& Path::SetExtension(const String& extension)
{
    internal_path.replace_extension({
        reinterpret_cast<const char8_t*>(extension.CStr())
    });
    return *this;
}

Path Path::operator/(const Path& other) const
{
    Path new_path = *this;
    new_path.Append(other);
    return new_path;
}

Path Path::WithFileName(const String& name) const
{
    Path new_path = *this;
    new_path.SetFileName(name);
    return new_path;
}

Path Path::WithExtension(const String& extension) const
{
    Path new_path = *this;
    new_path.SetExtension(extension);
    return new_path;
}

Path Path::GetNormalized() const
{
    Path new_path = *this;
    new_path.Normalize();
    return new_path;
}

Optional<Path> Path::RelativeTo(const Path& base) const
{
    std::filesystem::path relative = internal_path.lexically_relative(base.internal_path);

    if (relative.empty())
    {
        return std::nullopt;
    }
    return Path{ std::move(relative) };
}

Optional<Path> Path::Parent() const
{
    // 기본적으로 상위 경로가 없다고 판단되는 경우 (예: "filename", ".")
    if (!internal_path.has_parent_path())
    {
        return std::nullopt;
    }

    // 만약 .parent_path()를 해도 경로가 같다면, 이미 Root 경로인것으로 판정
    auto parent = internal_path.parent_path();
    if (parent == internal_path)
    {
        return std::nullopt;
    }
    return Path{ std::move(parent) };
}

Optional<String> Path::FileName() const
{
    if (!internal_path.has_filename())
    {
        return std::nullopt;
    }
    const std::u8string u8_str = internal_path.filename().generic_u8string();
    return StringUtils::ToString(u8_str);
}

Optional<String> Path::FileStem() const
{
    if (!internal_path.has_stem())
    {
        return std::nullopt;
    }

    const std::u8string u8_str = internal_path.stem().generic_u8string();
    return StringUtils::ToString(u8_str);
}

Optional<String> Path::Extension() const
{
    if (!internal_path.has_extension())
    {
        return std::nullopt;
    }

    const std::u8string u8_str = internal_path.extension().generic_u8string();
    return StringUtils::ToString(u8_str);
}

bool Path::IsEmpty() const
{
    return internal_path.empty();
}

bool Path::IsAbsolute() const
{
    return internal_path.is_absolute();
}

bool Path::IsRelative() const
{
    return internal_path.is_relative();
}

bool Path::IsSubPathOf(const Path& base) const
{
    // 상대 경로를 구했을 때 ".."으로 시작하면 하위 경로가 아님
    const std::filesystem::path relative = internal_path.lexically_relative(base.internal_path);

    if (relative.empty())
    {
        return false;
    }

    // 상대 경로의 첫 부분이 ".."인지 확인
    return *relative.begin() != "..";
}

bool Path::Exists() const
{
    std::error_code ec;
    return std::filesystem::exists(internal_path, ec);
}

bool Path::IsDirectory() const
{
    std::error_code ec;
    return std::filesystem::is_directory(internal_path, ec);
}

bool Path::IsFile() const
{
    std::error_code ec;
    return std::filesystem::is_regular_file(internal_path, ec);
}

String Path::ToString() const
{
    const std::u8string u8_str = internal_path.generic_u8string();
    return StringUtils::ToString(u8_str);
}

Optional<VPath> Path::ToVirtual() const
{
    return VFS::Get().Unresolve(*this);
}

void Path::Swap(Path& other) noexcept
{
    internal_path.swap(other.internal_path);
}

bool Path::operator==(const Path& other) const
{
    return internal_path == other.internal_path;
}

std::strong_ordering Path::operator<=>(const Path& other) const
{
    return internal_path <=> other.internal_path;
}
}  // namespace se
