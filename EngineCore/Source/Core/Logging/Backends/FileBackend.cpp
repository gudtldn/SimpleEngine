#include "SimpleEngine/Core/Logging/Backends/FileBackend.h"
#include "SimpleEngine/Core/FileSystem/FileSystem.h"


namespace se
{
FileBackend::FileBackend()
{
    const Path solution_path = PROJECT_ROOT_DIR;
    file_path = solution_path / "Logs/latest.log";
    OpenFile();
}

FileBackend::FileBackend(Path path)
{
    file_path = std::move(path);
    OpenFile();
}

void FileBackend::WriteLog(const LogEntry& entry)
{
    if (!file.is_open())
    {
        return;
    }

    const std::string formatted = std::format(
        "{} {:<7} {:<16} [{}:{}] {}\n",
        entry.GetTimestampString(),
        entry.GetLevelString(),
        entry.thread_name,
        entry.GetPrettyFileName(),
        entry.location.line(),
        entry.formatted_message
    );

    file << formatted;
    current_file_size += formatted.size();

    if (CheckRotation())
    {
        RotateFile();
    }
}

void FileBackend::Flush()
{
    if (file.is_open())
    {
        file.flush();
    }
}

void FileBackend::OpenFile()
{
    if (!file_path.Exists())
    {
        if (const Optional parent_opt = file_path.Parent())
        {
            FileSystem::CreateDirectories(*parent_opt);
        }
    }

    const String file_path_str = file_path.ToString();
    file.open(file_path_str.CStr(), std::ios::out | std::ios::app);
    if (file.is_open())
    {
        current_file_size = FileSystem::FileSize(file_path).ValueOr(0);
    }
}

void FileBackend::RotateFile()
{
    file.close();

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
}  // namespace se
