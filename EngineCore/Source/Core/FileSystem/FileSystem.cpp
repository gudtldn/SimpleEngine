#include "SimpleEngine/Core/FileSystem/FileSystem.h"

#include "SDL3/SDL.h"


#if SE_PLATFORM_WINDOWS
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#undef CreateDirectory
#endif

#include <filesystem>
#include <fstream>

#include "SimpleEngine/Utility/Common.h"


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

uint64 DirectoryEntry::LastWriteTime() const
{
    std::error_code ec;
    const auto time = internal_entry.last_write_time(ec);
    return ec ? 0 : time.time_since_epoch().count();
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
        return NullOpt;
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
    const std::filesystem::path std_from = ToStdPath(from);
    const std::filesystem::path std_to = ToStdPath(to);

#if SE_PLATFORM_WINDOWS
    // Windows에서는 std::filesystem::rename시,
    // 이미 같은 이름의 파일이 있을 경우 실패하기 때문에 ReplaceFileW를 대신 사용
    const std::wstring& wide_from = std_from.native();
    const std::wstring& wide_to = std_to.native();

    // ReplaceFileW 시도
    if (::ReplaceFileW(wide_to.c_str(), wide_from.c_str(), nullptr, REPLACEFILE_IGNORE_MERGE_ERRORS, nullptr, nullptr))
    {
        return true;
    }

    // ReplaceFileW가 실패한 경우 원인 파악
    const DWORD last_error = ::GetLastError();

    // 대상 파일이 없어서 실패한 것이라면, 단순 MoveFileEx로 처리 가능
    if (last_error == ERROR_FILE_NOT_FOUND || last_error == ERROR_PATH_NOT_FOUND)
    {
        return ::MoveFileExW(wide_from.c_str(), wide_to.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED);
    }

    // 그 외의 에러(권한, 공유 위반 등)는 실패로 처리
    return false;

#else
    std::error_code ec;
    std::filesystem::rename(std_from, std_to, ec);
    return !ec;
#endif
}

Optional<usize> FileSystem::FileSize(const Path& path)
{
    std::error_code ec;
    const auto size = std::filesystem::file_size(ToStdPath(path), ec);
    if (ec)
    {
        return NullOpt;
    }
    return static_cast<usize>(size);
}

Optional<uint64> FileSystem::LastWriteTime(const Path& path)
{
    std::error_code ec;
    const auto time = std::filesystem::last_write_time(ToStdPath(path), ec);
    if (ec)
    {
        return NullOpt;
    }
    return time.time_since_epoch().count();
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

std::generator<FileResult<ArrayView<const uint8>>> FileSystem::ReadChunked(Path path, usize chunk_size)
{
    const String path_str = path.ToString();

    // 바이너리 읽기모드
    SDL_IOStream* stream = SDL_IOFromFile(path_str.CStr(), "rb");

    // 파일 읽기 실패 처리
    if (!stream)
    {
        if (!path.Exists())
        {
            co_yield Unexpected{
                FileReadError::NotFound(String::Format("File not found: {}", path_str))
            };
        }
        else
        {
            co_yield Unexpected{
                FileReadError::OpenFailed(String::Format("Failed to open file: {} ({})", path_str, SDL_GetError()))
            };
        }
        co_return;
    }

    // 코루틴 종료 시, 파일 닫기
    SE_SCOPE_DEFER {
        SDL_CloseIO(stream);
    };

    // 청크 버퍼 할당
    Array<uint8> buffer;
    buffer.ResizeUninitialized(chunk_size);

    while (true)
    {
        const usize bytes_read = SDL_ReadIO(stream, buffer.Data(), chunk_size);

        if (bytes_read > 0)
        {
            co_yield ArrayView<const uint8>{ buffer.Data(), bytes_read };
        }

        if (bytes_read < chunk_size)
        {
            // 에러인지 단순 EOF인지 판별
            if (SDL_GetIOStatus(stream) == SDL_IO_STATUS_ERROR)
            {
                co_yield Unexpected{
                    FileReadError::Read(String::Format("Failed to read file: {} ({})", path_str, SDL_GetError()))
                };
            }
            break;
        }
    }
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
