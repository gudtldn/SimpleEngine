#include "SimpleEngine/Utility/SHA256.h"

#include "SimpleEngine/Core/FileSystem/FileSystem.h"
#include "SimpleEngine/Core/Logging/Logging.h"

#include "picosha2.h"


namespace se
{
namespace
{
ContentHash DigestFromHasher(picosha2::hash256_one_by_one& hasher)
{
    static_assert(ContentHash::DIGEST_SIZE == 32, "SHA-256 produces 32 bytes; update hash library if ContentHash size changes");

    hasher.finish();

    uint8 raw[ContentHash::DIGEST_SIZE];
    hasher.get_hash_bytes(raw, raw + ContentHash::DIGEST_SIZE);
    return ContentHash::FromRaw(raw);
}
} // namespace

ContentHash SHA256::HashFile(const Path& file_path)
{
    picosha2::hash256_one_by_one hasher;

    // 청크 사이즈 설정 (4MB)
    constexpr usize CHUNK_SIZE = 4ULL * 1024 * 1024;

    for (auto&& result : FileSystem::ReadChunked(file_path, CHUNK_SIZE))
    {
        if (result.HasError())
        {
            ConsoleLog(ELogLevel::Error, "SHA256::HashFile - {}", result.Error().What());
            return {};
        }

        const ArrayView<const uint8> chunk = result.Value();
        hasher.process(chunk.begin(), chunk.end());
    }

    return DigestFromHasher(hasher);
}

ContentHash SHA256::HashBytes(ArrayView<const uint8> data)
{
    picosha2::hash256_one_by_one hasher;
    hasher.process(data.begin(), data.end());
    return DigestFromHasher(hasher);
}

ContentHash SHA256::HashString(const StringView str)
{
    picosha2::hash256_one_by_one hasher;
    hasher.process(str.begin(), str.end());
    return DigestFromHasher(hasher);
}
} // namespace se
