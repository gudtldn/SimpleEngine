#include "Utility/FileUtils.h"

#include <fstream>

#include "Core/Container/StringView.h"
#include "Core/FileSystem/FileSystem.h"
#include "Utility/StringUtils.h"


namespace se
{
FileResult<Array<uint8>> FileIO::ReadBytes(const Path& file_path)
{
    const String path = file_path.ToString();

    std::ifstream file(path.CStr(), std::ios::binary);
    if (!file.is_open())
    {
        if (!file_path.Exists())
        {
            return Unexpected{ FileReadError::NotFound("File not found: " + path) };
        }
        if (!file_path.IsFile())
        {
            return Unexpected{ FileReadError::Format("File is not a regular file: " + path) };
        }
        return Unexpected{ FileReadError::OpenFailed("Failed to open file: " + path) };
    }

    const Optional<usize> file_size_opt = FileSystem::FileSize(file_path);
    if (!file_size_opt.HasValue())
    {
        return Unexpected{ FileReadError::EndOfFile("File size error: " + path) };
    }
    const usize file_size = *file_size_opt;

    Array<uint8> data(file_size);
    if (!file.read(reinterpret_cast<char*>(data.Data()), static_cast<isize>(file_size))) // TODO: 여기서 isize로 바꾸는 과정에서 overflow 가능성 있음
    {
        return Unexpected(FileReadError::Read("Failed to read file: " + path));
    }

    file.close();
    return data;
}

FileResult<String> FileIO::ReadString(const Path& file_path)
{
    const auto result = ReadBytes(file_path);
    if (result.HasValue())
    {
        return String{ reinterpret_cast<const char*>(result->Data()), result->Len() };
    }
    return Unexpected{ std::move(result).Error() };
}
}  // namespace se
