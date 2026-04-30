#include "SimpleEngine/Core/FileSystem/FileSystem.h"
#include "SimpleEngine/Utility/Common.h"

#include "SDL3/SDL.h"

#if SE_PLATFORM_WINDOWS
#ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
    #define NOMINMAX
#endif
#include <Windows.h>
#undef CreateDirectory

namespace
{
std::wstring ToWideString(const se::String& str)
{
    if (str.IsEmpty())
    {
        return {};
    }
    const int size_needed = MultiByteToWideChar(
        CP_UTF8, 0, str.Data(), static_cast<int>(str.ByteLen()),
        nullptr, 0
    );
    std::wstring wide(size_needed, 0);
    MultiByteToWideChar(
        CP_UTF8, 0, str.Data(), static_cast<int>(str.ByteLen()),
        wide.data(), size_needed
    );
    return wide;
}
} // namespace
#endif


namespace se
{
// =============================================================================
// DirectoryEntry
// =============================================================================

const Path& DirectoryEntry::GetPath() const
{
    return entry_path;
}

bool DirectoryEntry::IsDirectory() const
{
    return is_directory;
}

bool DirectoryEntry::IsFile() const
{
    return is_file;
}

bool DirectoryEntry::IsSymlink() const
{
    return false;
}

usize DirectoryEntry::FileSize() const
{
    return file_size;
}

uint64 DirectoryEntry::LastWriteTime() const
{
    return last_write_time;
}


// =============================================================================
// DirectoryIterator
// =============================================================================

// TODO: [Performance] 현재 생성자에서 모든 엔트리를 즉시 Array에 적재 (eager loading)하여,
//       대량 파일이 있는 디렉토리에서 메모리 사용량이 증가할 수 있음
//       필요 시 SDL_EnumerateDirectory 콜백 내에서 yield하는 lazy iterator로 전환 검토
DirectoryIterator::DirectoryIterator(const Path& path)
{
    struct Context
    {
        Array<DirectoryEntry>& entries;
        const Path& parent;
    };
    Context context{ .entries = entries, .parent = path };

    SDL_EnumerateDirectory(path.CStr(), [](void* userdata, const char* /*dirname*/, const char* fname) -> SDL_EnumerationResult
    {
        Context& ctx = *static_cast<Context*>(userdata);

        DirectoryEntry entry;
        entry.entry_path = ctx.parent / fname;

        SDL_PathInfo info;
        if (SDL_GetPathInfo(entry.entry_path.CStr(), &info))
        {
            entry.is_directory = info.type == SDL_PATHTYPE_DIRECTORY;
            entry.is_file = info.type == SDL_PATHTYPE_FILE;
            entry.file_size = static_cast<usize>(info.size);
            entry.last_write_time = static_cast<uint64>(info.modify_time);
        }

        ctx.entries.Push(std::move(entry));
        return SDL_ENUM_CONTINUE;
    }, &context);

    current_index = 0;
}

DirectoryIterator& DirectoryIterator::operator++()
{
    if (!IsEnd())
    {
        ++current_index;
    }
    return *this;
}

void DirectoryIterator::operator++(int)
{
    ++(*this);
}

bool DirectoryIterator::operator==(const DirectoryIterator& other) const
{
    if (IsEnd() && other.IsEnd())
    {
        return true;
    }
    if (IsEnd() != other.IsEnd())
    {
        return false;
    }
    return &entries == &other.entries && current_index == other.current_index;
}


// =============================================================================
// FileSystem
// =============================================================================

Path FileSystem::Absolute(const Path& path)
{
    if (path.IsEmpty())
    {
        return {};
    }
    if (path.IsAbsolute())
    {
        return path;
    }

    char* cwd = SDL_GetCurrentDirectory();
    if (!cwd)
    {
        return {};
    }

    Path result = cwd;
    SDL_free(cwd);
    result /= path;
    return result;
}

Optional<Path> FileSystem::Canonical(const Path& path)
{
    Path abs = Absolute(path);
    if (abs.IsEmpty() || !abs.Exists())
    {
        return NullOpt;
    }
    return abs;
}

bool FileSystem::CreateDirectories(const Path& path)
{
    if (path.IsEmpty())
    {
        return false;
    }
    return SDL_CreateDirectory(path.CStr());
}

bool FileSystem::Remove(const Path& path)
{
    if (path.IsEmpty())
    {
        return false;
    }
    if (!path.Exists())
    {
        return false;
    }
    return SDL_RemovePath(path.CStr());
}

usize FileSystem::RemoveAll(const Path& path)
{
    if (path.IsEmpty())
    {
        return 0;
    }

    SDL_PathInfo info;
    if (!SDL_GetPathInfo(path.CStr(), &info))
    {
        return 0;
    }

    usize count = 0;

    if (info.type == SDL_PATHTYPE_DIRECTORY)
    {
        for (const DirectoryEntry& entry : ReadDir(path))
        {
            count += RemoveAll(entry.GetPath());
        }
    }

    if (SDL_RemovePath(path.CStr()))
    {
        count++;
    }

    return count;
}

bool FileSystem::Copy(const Path& from, const Path& to)
{
    if (from.IsEmpty() || to.IsEmpty())
    {
        return false;
    }

    SDL_PathInfo info;
    if (!SDL_GetPathInfo(from.CStr(), &info))
    {
        return false;
    }

    if (info.type == SDL_PATHTYPE_FILE)
    {
        return SDL_CopyFile(from.CStr(), to.CStr());
    }
    if (info.type == SDL_PATHTYPE_DIRECTORY)
    {
        return SDL_CreateDirectory(to.CStr());
    }
    return false;
}

bool FileSystem::Rename(const Path& from, const Path& to)
{
    if (from.IsEmpty() || to.IsEmpty())
    {
        return false;
    }

#if SE_PLATFORM_WINDOWS
    // Windows에서는 ReplaceFileW로 원자적 덮어쓰기를 보장
    const std::wstring wide_from = ToWideString(from.ToString());
    const std::wstring wide_to = ToWideString(to.ToString());

    // ReplaceFileW 시도
    if (::ReplaceFileW(
        wide_to.c_str(), wide_from.c_str(),
        nullptr, REPLACEFILE_IGNORE_MERGE_ERRORS, nullptr, nullptr
    ))
    {
        return true;
    }

    // ReplaceFileW가 실패한 경우 원인 파악
    const DWORD last_error = ::GetLastError();

    // 대상 파일이 없어서 실패한 것이라면, 단순 MoveFileEx로 처리 가능
    if (last_error == ERROR_FILE_NOT_FOUND || last_error == ERROR_PATH_NOT_FOUND)
    {
        return ::MoveFileExW(
            wide_from.c_str(), wide_to.c_str(),
            MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED
        );
    }

    // 그 외의 에러(권한, 공유 위반 등)는 실패로 처리
    return false;

#else
    return SDL_RenamePath(from.CStr(), to.CStr());
#endif
}

bool FileSystem::Exists(const Path& path)
{
    if (path.IsEmpty())
    {
        return false;
    }

    SDL_PathInfo info;
    return SDL_GetPathInfo(path.CStr(), &info);
}

Optional<usize> FileSystem::FileSize(const Path& path)
{
    if (path.IsEmpty())
    {
        return NullOpt;
    }
    SDL_PathInfo info;
    if (!SDL_GetPathInfo(path.CStr(), &info))
    {
        return NullOpt;
    }
    return static_cast<usize>(info.size);
}

Optional<uint64> FileSystem::LastWriteTime(const Path& path)
{
    if (path.IsEmpty())
    {
        return NullOpt;
    }
    SDL_PathInfo info;
    if (!SDL_GetPathInfo(path.CStr(), &info))
    {
        return NullOpt;
    }
    return static_cast<uint64>(info.modify_time);
}

// TODO: [Performance] SDL_LoadFile가 내부적으로 malloc한 버퍼를 String 생성자에서 다시 복사함
//       String이 외부 버퍼 소유권을 직접 인수받는 생성자를 지원하면 복사 1회 절약 가능
FileResult<String> FileSystem::ReadToString(const Path& path)
{
    const String& path_str = path.ToString();

    size_t size = 0;
    void* data = SDL_LoadFile(path.CStr(), &size);
    if (!data)
    {
        if (!path.Exists())
        {
            return Unexpected<FileReadError>{ FileReadError::FileNotFound, "File not found: " + path_str };
        }
        return Unexpected<FileReadError>{ FileReadError::FileOpenFailed, "Failed to open file: " + path_str };
    }

    String content(static_cast<const char*>(data), static_cast<usize>(size));
    SDL_free(data);
    return content;
}

FileResult<Array<uint8>> FileSystem::ReadBytes(const Path& path)
{
    const String& path_str = path.ToString();

    size_t size = 0;
    void* data = SDL_LoadFile(path.CStr(), &size);
    if (!data)
    {
        if (!path.Exists())
        {
            return Unexpected<FileReadError>{ FileReadError::FileNotFound, "File not found: " + path_str };
        }
        return Unexpected<FileReadError>{ FileReadError::FileOpenFailed, "Failed to open file: " + path_str };
    }

    Array<uint8> result;
    result.ResizeUninitialized(static_cast<usize>(size));
    std::memcpy(result.Data(), data, size);
    SDL_free(data);
    return result;
}

std::generator<FileResult<ArrayView<const uint8>>> FileSystem::ReadChunked(Path path, usize chunk_size)
{
    const String& path_str = path.ToString();

    // 바이너리 읽기모드
    SDL_IOStream* stream = SDL_IOFromFile(path_str.CStr(), "rb");

    // 파일 읽기 실패 처리
    if (!stream)
    {
        if (!path.Exists())
        {
            co_yield Unexpected<FileReadError>{
                FileReadError::FileNotFound,
                String::Format("File not found: {}", path_str)
            };
        }
        else
        {
            co_yield Unexpected<FileReadError>{
                FileReadError::FileOpenFailed,
                String::Format("Failed to open file: {} ({})", path_str, SDL_GetError())
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
                co_yield Unexpected<FileReadError>{
                    FileReadError::ReadFailed,
                    String::Format("Failed to read file: {} ({})", path_str, SDL_GetError())
                };
            }
            break;
        }
    }
}

bool FileSystem::WriteString(const Path& path, StringView content)
{
    if (path.IsEmpty())
    {
        return false;
    }
    return SDL_SaveFile(path.CStr(), content.Data(), content.ByteLen());
}

bool FileSystem::Write(const Path& path, ArrayView<const uint8> data)
{
    if (path.IsEmpty())
    {
        return false;
    }
    return SDL_SaveFile(path.CStr(), data.Data(), data.Len());
}

DirectoryIterator FileSystem::ReadDir(const Path& path)
{
    return DirectoryIterator{ path };
}
} // namespace se
