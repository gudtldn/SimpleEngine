#include "SimpleEngine/Core/Logging/Backends/FileBackend.h"
#include "SimpleEngine/Core/FileSystem/FileSystem.h"
#include "SimpleEngine/Core/HAL/Platform.h"

#include "SDL3/SDL.h"


namespace se
{
FileBackend::FileBackend()
{
    const Path root_path = Platform::FindProjectRoot();
    file_path = root_path / "Logs/latest.log";
    OpenFile();
}

FileBackend::FileBackend(Path path)
{
    file_path = std::move(path);
    OpenFile();
}

FileBackend::~FileBackend()
{
    CloseFile();
}

void FileBackend::WriteLog(const LogEntry& entry)
{
    if (!io_stream)
    {
        return;
    }

    const String formatted = String::Format(
        "{} {:<7} {:<16} [{}:{}] {}\n",
        entry.GetTimestampString(),
        entry.GetLevelString(),
        entry.thread_name,
        entry.GetPrettyFileName(),
        entry.location.line(),
        entry.formatted_message
    );

    SDL_WriteIO(io_stream, formatted.Data(), formatted.ByteLen());
    current_file_size += formatted.ByteLen();

    if (CheckRotation())
    {
        RotateFile();
    }
}

void FileBackend::Flush()
{
    if (io_stream)
    {
        SDL_FlushIO(io_stream);
    }
}

void FileBackend::OpenFile()
{
    if (!file_path.Exists())
    {
        if (const auto parent = file_path.Parent())
        {
            FileSystem::CreateDirectories(*parent);
        }
    }

    io_stream = SDL_IOFromFile(file_path.CStr(), "ab");
    if (io_stream)
    {
        current_file_size = FileSystem::FileSize(file_path).ValueOr(0);
    }
}

void FileBackend::CloseFile()
{
    if (io_stream)
    {
        SDL_FlushIO(io_stream);
        SDL_CloseIO(io_stream);
        io_stream = nullptr;
    }
}

void FileBackend::RotateFile()
{
    CloseFile();

    namespace chrono = std::chrono;
    auto zt = chrono::zoned_time{ chrono::current_zone(), chrono::system_clock::now() };

    auto backup_path = file_path;
    backup_path += String::Format(".{:%Y-%m-%d_%H:%M:%S}", zt);

    FileSystem::Rename(file_path, backup_path);
    OpenFile();
    current_file_size = 0;
}

bool FileBackend::CheckRotation() const
{
    return current_file_size > max_file_size;
}
} // namespace se
