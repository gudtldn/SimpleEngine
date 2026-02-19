#include "SimpleEngine/Utility/SHA256.h"

#include "SimpleEngine/Core/FileSystem/FileSystem.h"
#include "SimpleEngine/Core/Logging/Logging.h"

#include "picosha2.h"

#include <string>


namespace se
{
String SHA256::HashFile(const Path& file_path)
{
    picosha2::hash256_one_by_one hasher;

    // 청크 사이즈 설정 (4MB)
    constexpr usize chunk_size = 4ULL * 1024 * 1024;

    const FileResult result = FileSystem::ReadChunked(file_path, chunk_size, [&hasher](ArrayView<const uint8> chunk)
    {
        hasher.process(chunk.begin(), chunk.end());
        return true;
    });

    if (!result)
    {
        ConsoleLog(ELogLevel::Error, "SHA256::HashFile - {}", result.Error().What());
        return {};
    }

    hasher.finish();

    std::string hex_str;
    picosha2::get_hash_hex_string(hasher, hex_str);

    return String::Format("sha256:{}", hex_str.c_str());
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
