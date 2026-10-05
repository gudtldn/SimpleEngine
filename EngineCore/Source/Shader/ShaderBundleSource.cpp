#include "SimpleEngine/Shader/ShaderBundleSource.h"

#include "SimpleEngine/Asset/DerivedDataCache.h"
#include "SimpleEngine/Utility/SHA256.h"

#include <algorithm>


namespace se
{
DdcShaderBundleSource::DdcShaderBundleSource(const DerivedDataCache& ddc)
    : ddc(ddc)
{
}

Expected<ShaderBundle, String> DdcShaderBundleSource::Load(const VPath& program) const
{
    const Guid key = KeyOf(program);

    // 쿡하지 않은 프로그램에서 Load가 읽기 실패 경고를 남기지 않도록 먼저 확인합니다.
    if (!ddc.Contains(key))
    {
        return Unexpected{ String::Format("Shader bundle is not cooked: {}", program.ToString()) };
    }

    const auto entry = ddc.Load(key);
    if (!entry)
    {
        return Unexpected{ String::Format("Failed to read shader bundle: {}", program.ToString()) };
    }
    if (entry->cache_version != ShaderBundle::FORMAT_VERSION)
    {
        return Unexpected{
            String::Format(
                "Shader bundle format version {} does not match {}: {}",
                entry->cache_version, ShaderBundle::FORMAT_VERSION, program.ToString()
            )
        };
    }

    auto bundle = ShaderBundle::Deserialize(entry->payload);
    if (!bundle)
    {
        return Unexpected{ String::Format("{}: {}", bundle.Error(), program.ToString()) };
    }
    return bundle;
}

Guid DdcShaderBundleSource::KeyOf(const VPath& program)
{
    const ContentHash hash = sha256::HashString(String::Format("ShaderBundle:{}", program.ToString()));

    FixedArray<u8, 16> bytes{};
    std::copy_n(hash.Data(), bytes.Len(), bytes.Data());

    // 이름 기반 UUID 버전 8(RFC 9562)과 RFC 변형 비트
    bytes[6] = static_cast<u8>((bytes[6] & 0x0F) | 0x80); // NOLINT(*-signed-bitwise)
    bytes[8] = static_cast<u8>((bytes[8] & 0x3F) | 0x80); // NOLINT(*-signed-bitwise)
    return Guid::FromBytes(bytes);
}
} // namespace se
