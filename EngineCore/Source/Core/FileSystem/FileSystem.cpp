#include "SimpleEngine/Core/FileSystem/FileSystem.h"

#include <filesystem>
#include <fstream>


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
// =============================================================================
// DirectoryEntry
// =============================================================================

DirectoryEntry::DirectoryEntry(std::filesystem::directory_entry entry)
    : internal_entry(std::move(entry))
{
}

Path DirectoryEntry::GetPath() const
{
    return Path{ internal_entry.path() };
}

bool DirectoryEntry::IsDirectory() const
{
    std::error_code ec;
    return internal_entry.is_directory(ec);
}

bool DirectoryEntry::IsFile() const
{
    std::error_code ec;
    return internal_entry.is_regular_file(ec);
}

bool DirectoryEntry::IsSymlink() const
{
    std::error_code ec;
    return internal_entry.is_symlink(ec);
}

usize DirectoryEntry::FileSize() const
{
    std::error_code ec;
    const auto size = internal_entry.file_size(ec);
    return ec ? 0 : static_cast<usize>(size);
}


// =============================================================================
// DirectoryIterator
// =============================================================================

DirectoryIterator::DirectoryIterator(const Path& path)
{
    std::error_code ec;
    internal_iter = std::filesystem::directory_iterator(ToStdPath(path), ec);
    if (!ec && internal_iter != std::filesystem::directory_iterator{})
    {
        is_end = false;
        current_entry = DirectoryEntry{ *internal_iter };
    }
}

DirectoryIterator& DirectoryIterator::operator++()
{
    ++internal_iter;
    if (internal_iter == std::filesystem::directory_iterator{})
    {
        is_end = true;
        current_entry = {};
    }
    else
    {
        current_entry = DirectoryEntry{ *internal_iter };
    }
    return *this;
}

DirectoryIterator DirectoryIterator::operator++(int)
{
    DirectoryIterator tmp = *this;
    ++(*this);
    return tmp;
}

bool DirectoryIterator::operator==(const DirectoryIterator& other) const
{
    // 둘 다 end이거나, 내부 이터레이터가 같으면 equal
    if (is_end && other.is_end)
    {
        return true;
    }
    if (is_end != other.is_end)
    {
        return false;
    }
    return internal_iter == other.internal_iter;
}


// =============================================================================
// FileSystem
// =============================================================================

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

FileResult<String> FileSystem::ReadToString(const Path& path)
{
    const String path_str = path.ToString();

    std::ifstream file(ToStdPath(path), std::ios::in | std::ios::binary);
    if (!file.is_open())
    {
        if (!path.Exists())
        {
            return Unexpected{ FileReadError::NotFound("File not found: " + path_str) };
        }
        return Unexpected{ FileReadError::OpenFailed("Failed to open file: " + path_str) };
    }

    file.seekg(0, std::ios::end);
    const auto size = file.tellg();
    file.seekg(0, std::ios::beg);

    if (size < 0)
    {
        return Unexpected{ FileReadError::EndOfFile("File size error: " + path_str) };
    }

    // 문자열로 읽기
    String content;
    content.ResizeForOverwrite(static_cast<usize>(size));
    file.read(content.Data(), size);

    if (file.fail() && !file.eof())
    {
        return Unexpected{ FileReadError::Read("Failed to read file: " + path_str) };
    }

    return content;
}

FileResult<Array<uint8>> FileSystem::ReadBytes(const Path& path)
{
    const String path_str = path.ToString();

    std::ifstream file(ToStdPath(path), std::ios::in | std::ios::binary);
    if (!file.is_open())
    {
        if (!path.Exists())
        {
            return Unexpected{ FileReadError::NotFound("File not found: " + path_str) };
        }
        return Unexpected{ FileReadError::OpenFailed("Failed to open file: " + path_str) };
    }

    file.seekg(0, std::ios::end);
    const auto size = file.tellg();
    file.seekg(0, std::ios::beg);

    if (size < 0)
    {
        return Unexpected{ FileReadError::EndOfFile("File size error: " + path_str) };
    }

    // 바이트 배열로 읽기
    Array<uint8> data;
    data.Resize(static_cast<usize>(size));
    file.read(reinterpret_cast<char*>(data.Data()), size);

    if (file.fail() && !file.eof())
    {
        return Unexpected{ FileReadError::Read("Failed to read file: " + path_str) };
    }

    return data;
}

FileResult<void> FileSystem::ReadChunked(const Path& path, usize chunk_size, const Function<bool(ArrayView<const uint8>)>& callback)
{
    const String path_str = path.ToString();

    std::ifstream file(ToStdPath(path), std::ios::in | std::ios::binary);
    if (!file.is_open())
    {
        if (!path.Exists())
        {
            return Unexpected{ FileReadError::NotFound("File not found: " + path_str) };
        }
        return Unexpected{ FileReadError::OpenFailed("Failed to open file: " + path_str) };
    }

    // 청크 버퍼 할당
    Array<uint8> buffer;
    buffer.ResizeUninitialized(chunk_size);

    while (file)
    {
        // 파일에서 청크 사이즈만큼 읽기 시도
        file.read(reinterpret_cast<char*>(buffer.Data()), static_cast<std::streamsize>(chunk_size));
        const std::streamsize bytes_read = file.gcount();

        if (bytes_read > 0)
        {
            // 실제 읽어들인 크기만큼 ArrayView를 만들어 Callback으로 전달
            ArrayView<const uint8> chunk_view(buffer.Data(), static_cast<usize>(bytes_read));
            if (!callback(chunk_view))
            {
                break;
            }
        }

        if (file.eof())
        {
            break;
        }

        if (file.fail())
        {
            return Unexpected{ FileReadError::Read("Failed to read file: " + path_str) };
        }
    }

    return {};
}

bool FileSystem::WriteString(const Path& path, StringView content)
{
    std::ofstream file(ToStdPath(path), std::ios::out | std::ios::binary | std::ios::trunc);
    if (!file.is_open())
    {
        return false;
    }

    file.write(content.Data(), static_cast<std::streamsize>(content.ByteLen()));
    return !file.fail();
}

bool FileSystem::Write(const Path& path, ArrayView<const uint8> data)
{
    std::ofstream file(ToStdPath(path), std::ios::out | std::ios::binary | std::ios::trunc);
    if (!file.is_open())
    {
        return false;
    }

    file.write(reinterpret_cast<const char*>(data.Data()), static_cast<std::streamsize>(data.Len()));
    return !file.fail();
}

DirectoryIterator FileSystem::ReadDir(const Path& path)
{
    return DirectoryIterator{ path };
}
} // namespace se
