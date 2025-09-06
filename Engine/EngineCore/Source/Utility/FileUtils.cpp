module SE.Utility;
import :StringUtils;
import :FileUtils;


namespace se::utility::file_utils
{
FileResult<vector<uint8>> ReadToByteArray(const std::filesystem::path& file_path)
{
    const u8string u8_path(file_path.generic_u8string());

    std::ifstream file(file_path, std::ios::binary);
    if (!file.is_open())
    {
        if (!std::filesystem::exists(file_path))
        {
            return std::unexpected{ FileReadError::NotFound(u8"File not found: " + u8_path) };
        }
        if (!std::filesystem::is_regular_file(file_path))
        {
            return std::unexpected{ FileReadError::Format(u8"File is not a regular file: " + u8_path) };
        }
        return std::unexpected{ FileReadError::OpenFailed(u8"Failed to open file: " + u8_path) };
    }

    std::error_code ec;
    const uint64 file_size = std::filesystem::file_size(file_path, ec);
    if (ec)
    {
        return std::unexpected{ FileReadError::EOF(u8"File size error: " + u8_path) };
    }

    vector<uint8> data(file_size);
    if (!file.read(reinterpret_cast<char*>(data.data()), file_size))
    {
        return std::unexpected(FileReadError::Read(u8"Failed to read file: " + u8_path));
    }

    file.close();
    return data;
}

FileResult<u8string> ReadToString(const std::filesystem::path& file_path)
{
    const auto result = ReadToByteArray(file_path);
    if (result.has_value())
    {
        return string_utils::ToU8String({ reinterpret_cast<const char*>(result->data()), result->size() });
    }
    return std::unexpected{ std::move(result).error() };
}
}
