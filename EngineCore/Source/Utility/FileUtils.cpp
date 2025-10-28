#include "Utility/FileUtils.h"

#include <fstream>

#include "Utility/StringUtils.h"


namespace se::utility::file
{
FileResult<Array<uint8>> ReadToByteArray(const std::filesystem::path& file_path)
{
    const String path(file_path.generic_string());

    std::ifstream file(file_path, std::ios::binary);
    if (!file.is_open())
    {
        if (!std::filesystem::exists(file_path))
        {
            return std::unexpected{ FileReadError::NotFound("File not found: " + path) };
        }
        if (!std::filesystem::is_regular_file(file_path))
        {
            return std::unexpected{ FileReadError::Format("File is not a regular file: " + path) };
        }
        return std::unexpected{ FileReadError::OpenFailed("Failed to open file: " + path) };
    }

    std::error_code ec;
    const usize file_size = std::filesystem::file_size(file_path, ec);
    if (ec)
    {
        return std::unexpected{ FileReadError::EndOfFile("File size error: " + path) };
    }

    Array<uint8> data(file_size);
    if (!file.read(reinterpret_cast<char*>(data.Data()), static_cast<isize>(file_size))) // TODO: 여기서 isize로 바꾸는 과정에서 overflow 가능성 있음
    {
        return std::unexpected(FileReadError::Read("Failed to read file: " + path));
    }

    file.close();
    return data;
}

FileResult<String> ReadToString(const std::filesystem::path& file_path)
{
    const auto result = ReadToByteArray(file_path);
    if (result.has_value())
    {
        return String{ std::string_view{ reinterpret_cast<const char*>(result->Data()), result->Len() } };
    }
    return std::unexpected{ std::move(result).error() };
}
}
