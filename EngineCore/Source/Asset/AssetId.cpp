#include "SimpleEngine/Asset/AssetId.h"

#include "SimpleEngine/Core/Serialization/BuiltinTraits.h"
#include "SimpleEngine/Core/Serialization/SerializeOpsRegistry.h"


namespace se
{
const AssetId AssetId::invalid = {};

/** Guid 트레이트로 저장합니다. 텍스트 포맷에서는 GUID 문자열, 바이너리에서는 16바이트이고, 무효 AssetId는 0으로 채운 GUID입니다. */
template <>
struct SerializeTraits<AssetId>
{
    /** Guid 트레이트의 표현을 그대로 쓰므로 버전도 따라갑니다. */
    static constexpr u32 FORMAT_VERSION = SerializeTraits<Guid>::FORMAT_VERSION;

    static void Write(ArchiveWriter& writer, const AssetId& value)
    {
        SerializeTraits<Guid>::Write(writer, value.GetGuid());
    }

    static void Read(ArchiveReader& reader, AssetId& value)
    {
        Guid guid;
        SerializeTraits<Guid>::Read(reader, guid);
        if (reader.HasError())
        {
            return;
        }
        value = AssetId{ guid };
    }
};
} // namespace se

SE_REGISTER_SERIALIZE_TRAITS(se::AssetId)
