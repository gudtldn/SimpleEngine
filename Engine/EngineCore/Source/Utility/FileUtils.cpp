module SimpleEngine.Utility;
import :FileUtils;


namespace se::utility::file_utils
{
FileResult<std::vector<uint8>> ReadFromBinary(const std::filesystem::path& file_path)
{
    std::ifstream file(file_path, std::ios::ate | std::ios::binary);
    if (!file.is_open())
    {
        return std::unexpected{ u8"Failed to open shader file: " + file_path.generic_u8string() };
    }

    const size_t size = file.tellg();
    if (size == 0)
    {
        return std::unexpected{ u8"Shader file is empty: " + file_path.generic_u8string() };
    }

    std::vector<uint8> data(size);
    file.seekg(0);
    file.read(reinterpret_cast<char*>(data.data()), size);
    file.close();

    return data;
}
}
