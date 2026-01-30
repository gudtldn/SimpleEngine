#include "Utility/FileSystem.h"

#include <filesystem>


namespace
{
std::filesystem::path ToStdPath(const se::Path& path)
{
    const se::String str = path.ToString();
    return std::filesystem::path{ reinterpret_cast<const char8_t*>(str.CStr()) };
}
}  // namespace


namespace se
{
Path FileSystem::Absolute(const Path& path)
{
    std::error_code ec;
    std::filesystem::path result = std::filesystem::absolute(ToStdPath(path), ec);
    if (ec)
    {
        return {};
    }
    return Path{ std::move(result) };
}

Optional<Path> FileSystem::Canonical(const Path& path)
{
    std::error_code ec;
    std::filesystem::path result = std::filesystem::canonical(ToStdPath(path), ec);
    if (ec)
    {
        return std::nullopt;
    }
    return Path{ std::move(result) };
}

bool FileSystem::CreateDirectory(const Path& path)
{
    std::error_code ec;
    return std::filesystem::create_directory(ToStdPath(path), ec) || !ec;
}

bool FileSystem::CreateDirectories(const Path& path)
{
    std::error_code ec;
    return std::filesystem::create_directories(ToStdPath(path), ec) || !ec;
}

bool FileSystem::Remove(const Path& path)
{
    std::error_code ec;
    return std::filesystem::remove(ToStdPath(path), ec);
}

usize FileSystem::RemoveAll(const Path& path)
{
    std::error_code ec;
    return std::filesystem::remove_all(ToStdPath(path), ec);
}

bool FileSystem::Copy(const Path& from, const Path& to)
{
    std::error_code ec;
    std::filesystem::copy(ToStdPath(from), ToStdPath(to), ec);
    return !ec;
}

bool FileSystem::Rename(const Path& from, const Path& to)
{
    std::error_code ec;
    std::filesystem::rename(ToStdPath(from), ToStdPath(to), ec);
    return !ec;
}

Optional<usize> FileSystem::FileSize(const Path& path)
{
    std::error_code ec;
    const auto size = std::filesystem::file_size(ToStdPath(path), ec);
    if (ec)
    {
        return std::nullopt;
    }
    return static_cast<usize>(size);
}

}  // namespace se
