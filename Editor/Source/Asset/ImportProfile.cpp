#include "SimpleEditor/Asset/ImportProfile.h"

#include "SimpleEngine/Core/Logging/Logging.h"
#include "SimpleEngine/Core/Reflection/Rtti.h"
#include "SimpleEngine/Core/Reflection/TypeRecordRegistry.h"
#include "SimpleEngine/Core/Reflection/TypeRegistry.h"
#include "SimpleEngine/Core/Reflection/ValueOpsRegistry.h"
#include "SimpleEngine/Core/Serialization/BinaryArchive.h"
#include "SimpleEngine/Core/Serialization/SerializeContext.h"
#include "SimpleEngine/Core/Serialization/SerializeOpsRegistry.h"
#include "SimpleEngine/Core/Serialization/SerializePlanRegistry.h"
#include "SimpleEngine/Core/Serialization/Serializer.h"
#include "SimpleEngine/Core/Types/StringName.h"
#include "SimpleEngine/Utility/SHA256.h"


namespace se
{
namespace
{
/** 설정 타입 이름으로 등록된 TypeInfo를 찾습니다. 이름이 정규 이름과 다르면 NullOpt입니다. */
[[nodiscard]] Optional<const TypeInfo&> FindSettingsType(StringView name)
{
    const auto info = TypeRegistry::Get().Find(TypeId::FromCanonicalName(name));
    if (!info.HasValue() || info->name != name)
    {
        return NullOpt;
    }
    return info;
}

/**
 * 모르는 설정 타입의 맵 엔트리를 value를 읽지 않고 건너뛰고, SerializeContext에 SkippedImportSettings가 있으면 이름을 기록합니다.
 * 태그 없는 바이너리 포맷은 value의 크기를 몰라 건너뛸 수 없으므로 오류로 처리합니다.
 */
void SkipSettingsEntry(ArchiveReader& reader, StringView name)
{
    if (!reader.IsTextFormat())
    {
        reader.SetError(String::Format("SerializeTraits<ImportProfile>: unknown import settings type '{}' cannot be skipped in a binary format.", name));
        return;
    }

    // 텍스트 reader는 value를 읽지 않아도 EndMapEntry에서 다음 엔트리로 넘어감
    if (const SerializeContext* const context = reader.GetContext())
    {
        if (editor::SkippedImportSettings* const skipped = context->Find<editor::SkippedImportSettings>())
        {
            skipped->type_names.Push(name);
        }
    }
}

/**
 * 맵 엔트리 하나에서 설정 타입 이름과 그 설정을 읽어 settings_map에 넣습니다.
 * 등록된 ImportSettingsBase 파생 타입이 아니면 SkipSettingsEntry로 건너뜁니다.
 */
void ReadSettingsEntry(ArchiveReader& reader, editor::ImportProfile::SettingsMap& settings_map)
{
    String name;
    reader.Str(name);
    if (reader.HasError())
    {
        return;
    }

    const auto info = FindSettingsType(name);
    if (!info || !IsA<editor::ImportSettingsBase>(info->id))
    {
        SkipSettingsEntry(reader, name);
        return;
    }
    const auto ops = ValueOpsRegistry::Get().Find(info->id);
    const auto record = TypeRecordRegistry::Get().Find(info->id);
    if (!ops || !ops->new_object || !record)
    {
        SkipSettingsEntry(reader, name);
        return;
    }

    const auto plan = SerializePlanRegistry::Get().FindOrCompile(info->id);
    if (plan.HasError())
    {
        reader.SetError(String::Format("SerializeTraits<ImportProfile>: {}.", plan.Error()));
        return;
    }

    // new_object는 new T로 만들므로 ImportSettingsBase의 가상 소멸자로 해제할 수 있음
    void* const raw = ops->new_object();
    std::shared_ptr<editor::ImportSettingsBase> settings{
        static_cast<editor::ImportSettingsBase*>(CastById(raw, info->id, TypeId::Of<editor::ImportSettingsBase>(), *record))
    };
    if (serde::Deserialize(reader, *plan.Value(), raw).HasError())
    {
        return;
    }
    settings_map.Insert(info->id, std::move(settings));
}
} // namespace

/**
 * 설정 타입 이름을 key로, 그 타입의 필드를 value로 하는 맵으로 저장합니다. 기존 .meta 파일의 import_settings 테이블과 같은 표현입니다.
 * 읽을 때 객체는 ValueOps로 만들고 필드는 직렬화로 채우며, 모르는 설정 타입은 건너뜁니다.
 */
template <>
struct SerializeTraits<editor::ImportProfile>
{
    static constexpr u32 FORMAT_VERSION = 1;

    static void Write(ArchiveWriter& writer, const editor::ImportProfile& value)
    {
        // HashMap 순서는 삽입 이력에 따라 달라지므로, 같은 설정이 같은 바이트가 되도록 타입 이름 순으로 씀
        Array<TypeId> type_ids = value.GetSettingsMap().Keys();
        type_ids.SortBy([](TypeId type_id) { return TypeRegistry::Get().FindChecked(type_id).name; });

        writer.BeginMap(type_ids.Len());
        for (const TypeId type_id : type_ids)
        {
            const StringView name = TypeRegistry::Get().FindChecked(type_id).name;
            const auto plan = SerializePlanRegistry::Get().FindOrCompile(type_id);
            if (plan.HasError())
            {
                writer.SetError(String::Format("SerializeTraits<ImportProfile>: {}.", plan.Error()));
                return;
            }

            // 필드 오프셋은 가장 파생된 타입 기준이므로, ImportSettingsBase 서브오브젝트가 아닌 완전 객체의 주소를 넘김
            const editor::ImportSettingsBase& settings = *value.GetSettingsMap()[type_id];
            const void* const complete = CompleteObjectOf(&settings);
            if (complete == nullptr)
            {
                writer.SetError(String::Format("SerializeTraits<ImportProfile>: ImportSettingsBase is not a single SE_BASE of '{}'.", name));
                return;
            }

            writer.BeginMapEntry();
            writer.Str(name);
            if (serde::Serialize(writer, *plan.Value(), complete).HasError())
            {
                return;
            }
            writer.EndMapEntry();
        }
        writer.EndMap();
    }

    static void Read(ArchiveReader& reader, editor::ImportProfile& value)
    {
        u64 count = 0;
        reader.BeginMap(count);
        if (reader.HasError())
        {
            return;
        }

        value.settings_map.Clear();
        for (usize i = 0; i < static_cast<usize>(count); ++i)
        {
            reader.BeginMapEntry();
            ReadSettingsEntry(reader, value.settings_map);
            reader.EndMapEntry();
            if (reader.HasError())
            {
                return;
            }
        }
        reader.EndMap();
    }
};
} // namespace se

SE_REGISTER_SERIALIZE_TRAITS(se::editor::ImportProfile)


namespace se::editor
{
Expected<ContentHash, String> ImportProfile::ComputeSettingsHash() const
{
    Array<u8> bytes;
    BinaryWriter writer(bytes);
    if (const auto result = serde::Serialize(writer, *this); result.HasError())
    {
        return Unexpected{ String::Format("failed to serialize import settings: {}", result.Error().message) };
    }
    return sha256::HashBytes(bytes);
}
} // namespace se::editor
