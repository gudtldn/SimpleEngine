#include "SimpleEngine/Utility/SHA256.h"

#include "SimpleEngine/Core/FileSystem/FileSystem.h"
#include "SimpleEngine/Core/Logging/Logging.h"

#include "picosha2.h"

#include <string>


namespace se
{
String SHA256::HashFile(const Path& file_path)
{
    const FileResult<Array<uint8>> result = FileSystem::ReadBytes(file_path);
    if (!result.HasValue())
    {
        ConsoleLog(ELogLevel::Error, "SHA256::HashFile - Failed to read file: {}, ", file_path, result.Error().What());
        return {};
    }

    return HashBytes(result.Value());
}

String SHA256::HashBytes(const Array<uint8>& data)
{
    std::string hex_str;
    picosha2::hash256_hex_string(
        data.begin(), data.end(),
        hex_str
    );
    return String::Format("sha256:{}", hex_str.c_str());
}

String SHA256::HashString(const StringView str)
{
    const uint8* begin = reinterpret_cast<const uint8*>(str.Data());
    const uint8* end = begin + str.ByteLen();

    std::string hex_str;
    picosha2::hash256_hex_string(begin, end, hex_str);
    return String::Format("sha256:{}", hex_str.c_str());
}
}  // namespace se
