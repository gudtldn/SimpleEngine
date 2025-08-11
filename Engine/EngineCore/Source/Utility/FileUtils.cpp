module SimpleEngine.Utility;
import :FileUtils;


namespace se::utility::file_utils
{
FileResult<std::vector<uint8>> ReadFromBinary(const std::filesystem::path& file_path)
{
    if (!std::filesystem::exists(file_path))
    {
        return std::unexpected{ FileReadError::NotFound(u8"File not found: " + file_path.generic_u8string()) };
    }

    std::ifstream file(file_path, std::ios::ate | std::ios::binary);
    if (!file.is_open())
    {
        return std::unexpected{ FileReadError::OpenFailed(u8"Failed to open file: " + file_path.generic_u8string()) };
    }

    const size_t size = file.tellg();
    if (size == 0)
    {
        return std::unexpected(FileReadError::EOF(u8"File is empty: " + file_path.generic_u8string()));
    }

    std::vector<uint8> data(size);
    file.seekg(0);

    if (!file.read(reinterpret_cast<char*>(data.data()), size))
    {
        return std::unexpected(FileReadError::Read(u8"Failed to read file: " + file_path.generic_u8string()));
    }

    file.close();
    return data;
}
}
