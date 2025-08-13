module SimpleEngine.Core;
import :Logging.Backends.FileBackend;


namespace se::core::logging::backends
{
FileBackend::FileBackend()
{
    const std::filesystem::path solution_path = std::filesystem::current_path().parent_path().parent_path();
    file_path = solution_path / "Logs/latest.log";
    OpenFile();
}

FileBackend::FileBackend(std::filesystem::path path)
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
        "{} {:<7} [{}:{}] {}\n",
        entry.GetTimestampString(), entry.GetLevelString(), entry.GetPrettyFileName(), entry.location.line(), entry.formatted_message
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
    if (!std::filesystem::exists(file_path))
    {
        std::filesystem::create_directories(file_path.parent_path());
    }

    file.open(file_path, std::ios::out | std::ios::app);
    if (file.is_open())
    {
        current_file_size = std::filesystem::file_size(file_path);
    }
}

void FileBackend::RotateFile()
{
    file.close();

    namespace chrono = std::chrono;
    auto zt = chrono::zoned_time{ chrono::current_zone(), chrono::system_clock::now() };;

    auto backup_path = file_path;
    backup_path += std::format(".{:%Y-%m-%d_%H:%M:%S}", zt);

    std::filesystem::rename(file_path, backup_path);
    OpenFile();
    current_file_size = 0;
}

bool FileBackend::CheckRotation() const
{
    return current_file_size > max_file_size;
}
}
