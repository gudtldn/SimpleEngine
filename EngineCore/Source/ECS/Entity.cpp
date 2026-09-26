#include "SimpleEngine/ECS/Entity.h"

#include "SimpleEngine/Core/Serialization/SerializeContext.h"
#include "SimpleEngine/Core/Serialization/SerializeOpsRegistry.h"
#include "SimpleEngine/ECS/EntityRemapper.h"


namespace se
{
namespace
{
/** archive의 SerializeContext에서 EntityRemapper를 찾습니다. context나 EntityRemapper가 없으면 archive에 오류를 남기고 nullptr를 돌려줍니다. */
[[nodiscard]] EntityRemapper* FindRemapper(Archive& archive)
{
    SerializeContext* const context = archive.GetContext();
    if (context == nullptr)
    {
        archive.SetError("SerializeTraits<Entity>: the archive has no SerializeContext. Add an EntityRemapper to a SerializeContext and pass it with SetContext.");
        return nullptr;
    }

    EntityRemapper* const remapper = context->Find<EntityRemapper>();
    if (remapper == nullptr)
    {
        archive.SetError("SerializeTraits<Entity>: the SerializeContext has no EntityRemapper.");
        return nullptr;
    }
    return remapper;
}
} // namespace

/**
 * 실행마다 달라지는 슬롯 번호(id, generation) 대신 EntityRemapper가 정한 u64 영속 ID로 저장합니다. null Entity는 0입니다.
 * 로드할 때 EntityRemapper가 모르는 영속 ID는 null Entity로 읽고, 로더가 알릴 수 있게 EntityRemapper에 기록됩니다.
 */
template <>
struct SerializeTraits<Entity>
{
    static constexpr u32 FORMAT_VERSION = 1;

    static void Write(ArchiveWriter& writer, const Entity& value)
    {
        EntityRemapper* const remapper = FindRemapper(writer);
        if (remapper == nullptr)
        {
            return;
        }

        // 저장하지 않는 엔티티를 가리키는 참조는 영속 ID로 쓸 수 없으므로 오류로 처리
        const auto persistent_id = remapper->ToPersistentId(value);
        if (!persistent_id)
        {
            writer.SetError(String::Format(
                "SerializeTraits<Entity>: entity (id {}, generation {}) has no persistent id in the EntityRemapper.",
                value.GetId(), value.GetGeneration()));
            return;
        }
        writer.Int(static_cast<i64>(*persistent_id), EIntWidth::Bits64, false);
    }

    static void Read(ArchiveReader& reader, Entity& value)
    {
        EntityRemapper* const remapper = FindRemapper(reader);
        if (remapper == nullptr)
        {
            return;
        }

        i64 persistent_id = 0;
        reader.Int(persistent_id, EIntWidth::Bits64, false);
        if (reader.HasError())
        {
            return;
        }
        value = remapper->ToEntity(static_cast<u64>(persistent_id));
    }
};
} // namespace se

SE_REGISTER_SERIALIZE_TRAITS(se::Entity)
