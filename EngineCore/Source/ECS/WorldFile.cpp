#include "SimpleEngine/ECS/WorldFile.h"

#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Container/HashSet.h"
#include "SimpleEngine/Core/Math/Random.h"
#include "../../Include/SimpleEngine/Core/Reflection/Legacy/TypeRegistry.h"
#include "SimpleEngine/Core/Reflection/TypeRegistry.h"
#include "SimpleEngine/Core/Serialization/Archive.h"
#include "SimpleEngine/Core/Serialization/SerializeContext.h"
#include "SimpleEngine/Core/Serialization/SerializePlanRegistry.h"
#include "SimpleEngine/Core/Serialization/Serializer.h"
#include "SimpleEngine/Core/Serialization/Transient.h"
#include "SimpleEngine/ECS/Components/PersistentIdComponent.h"
#include "SimpleEngine/ECS/ECSRegistry.h"
#include "SimpleEngine/ECS/EntityRemapper.h"
#include "SimpleEngine/ECS/World.h"
#include "SimpleEngine/Utility/Common.h"

#include <ranges>


namespace se
{
namespace
{
/** 월드 파일 문서 구조(루트, 엔티티 목록)의 버전 */
constexpr i64 FORMAT_VERSION = 1;

/** 영속 ID를 JSON 숫자로 정확히 쓸 수 있게 하위 53비트만 남기는 마스크 */
constexpr u64 PERSISTENT_ID_MASK = (u64{ 1 } << 53) - 1;

/** Read가 파일의 엔티티 하나로 만든 엔티티 */
struct LoadedEntity
{
    Entity entity;

    /** 파일에 적힌 영속 ID. 파일 안의 Entity 참조는 이 ID로 엔티티를 가리킵니다. */
    u64 file_id = 0;
};

/** Write가 저장할 컴포넌트 타입 하나 */
struct SavedComponent
{
    /** 파일에서 컴포넌트를 가리키는 타입 이름 */
    String name;

    const IComponentStorage* storage = nullptr;
    const SerializePlan* plan = nullptr;

    /** 저장할 수 없는 이유. 비어 있으면 저장할 수 있습니다. */
    String error;
};

/** Read가 파일의 타입 이름으로 찾은 컴포넌트 타입 하나 */
struct ReadableComponent
{
    enum class EKind : u8
    {
        /** ECS에 등록되지 않았거나 새 리플렉션에 없는 타입 */
        Unknown,

        /** Transient가 붙어 파일에 저장하지 않는 타입 */
        Transient,

        /** 읽어서 world에 붙일 수 있는 타입 */
        Saved,
    };

    EKind kind = EKind::Unknown;
    const ComponentOps* ops = nullptr;
    const SerializePlan* plan = nullptr;
};

/** used_ids에 없는 영속 ID를 무작위로 만들어 used_ids에 넣고 돌려줍니다. 범위는 [1, 2^53)입니다. */
[[nodiscard]] u64 NewPersistentId(HashSet<u64>& used_ids)
{
    while (true)
    {
        const u64 high = random::Next();
        const u64 id = ((high << 32) | random::Next()) & PERSISTENT_ID_MASK;
        if (id != 0 && used_ids.Insert(id))
        {
            return id;
        }
    }
}

/** world의 엔티티에 붙은 영속 ID를 모읍니다. 아직 ID가 없다는 뜻인 0은 뺍니다. */
[[nodiscard]] HashSet<u64> CollectPersistentIds(const World& world)
{
    HashSet<u64> ids;
    if (const auto storage = world.FindSparseSet<PersistentIdComponent>())
    {
        for (const PersistentIdComponent& component : storage->GetComponents())
        {
            if (component.id != 0)
            {
                ids.Insert(component.id);
            }
        }
    }
    return ids;
}

/** 새 리플렉션 TypeId로 ECSRegistry에 등록된 컴포넌트의 ComponentOps를 찾습니다. */
[[nodiscard]] Optional<const ComponentOps&> FindComponentOps(TypeId type)
{
    for (const ComponentOps& ops : ECSRegistry::Get().GetComponentOpsMap() | std::views::values)
    {
        if (ops.type == type)
        {
            return ops;
        }
    }
    return NullOpt;
}

/** 오류 메시지에 쓸 레거시 타입 이름을 돌려줍니다. 레거시 리플렉션에도 없는 타입이면 해시를 씁니다. */
[[nodiscard]] String LegacyTypeNameOf(const TypeId_v1& type_id)
{
    if (const auto info = TypeRegistry_v1::Get().Find(type_id))
    {
        return { info->name };
    }
    return String::Format("type hash {:#x}", type_id.GetHash());
}

/**
 * 저장소 하나의 컴포넌트를 저장하는 데 필요한 정보를 만듭니다. Transient가 붙은 타입은 저장하지 않으므로 NullOpt입니다.
 * 등록되지 않은 타입이라 저장할 수 없으면 그 이유를 error에 담습니다.
 */
[[nodiscard]] Optional<SavedComponent> MakeSavedComponent(const TypeId_v1& legacy_type, const IComponentStorage& storage)
{
    const auto ops = ECSRegistry::Get().GetComponentOps(legacy_type);
    if (!ops)
    {
        return SavedComponent{
            .storage = &storage,
            .error = String::Format("component '{}' is not registered as an ECS component (meta::Component).", LegacyTypeNameOf(legacy_type)),
        };
    }

    // 등록이 없는 컴포넌트를 건너뛰면 파일에서 조용히 사라지므로 오류로 처리
    const auto info = TypeRegistry::Get().Find(ops->type);
    if (!info)
    {
        return SavedComponent{
            .storage = &storage,
            .error = String::Format(
                "component '{}' is not registered with SE_REFLECT_BEGIN. Register its fields, or annotate it with Transient if it must not be saved.",
                LegacyTypeNameOf(legacy_type)),
        };
    }
    if (info->HasAnnotation<TransientAnnotation>())
    {
        return NullOpt;
    }

    const auto plan = SerializePlanRegistry::Get().FindOrCompile(ops->type);
    if (!plan)
    {
        return SavedComponent{ .storage = &storage, .error = String::Format("component '{}': {}", info->name, plan.Error()) };
    }
    return SavedComponent{ .name = String{ info->name }, .storage = &storage, .plan = plan.Value() };
}

/** type이 가리키는 컴포넌트 타입을 찾습니다. 읽을 수 없는 타입은 종류만 알려 주고, Plan을 만들지 못하면 오류입니다. */
[[nodiscard]] Expected<ReadableComponent, String> ResolveComponent(TypeId type, StringView type_name)
{
    const auto info = TypeRegistry::Get().Find(type);
    const auto ops = FindComponentOps(type);
    if (!info || !ops)
    {
        return ReadableComponent{ .kind = ReadableComponent::EKind::Unknown };
    }
    if (info->HasAnnotation<TransientAnnotation>())
    {
        return ReadableComponent{ .kind = ReadableComponent::EKind::Transient };
    }

    const auto plan = SerializePlanRegistry::Get().FindOrCompile(type);
    if (!plan)
    {
        return Unexpected{ String::Format("component '{}': {}", type_name, plan.Error()) };
    }
    return ReadableComponent{ .kind = ReadableComponent::EKind::Saved, .ops = &*ops, .plan = plan.Value() };
}

/** cache에서 type의 컴포넌트 정보를 찾고, 없으면 처음 한 번 만들어 cache에 넣습니다. */
[[nodiscard]] Expected<ReadableComponent, String> FindOrResolveComponent(HashMap<TypeId, ReadableComponent>& cache, TypeId type, StringView type_name)
{
    if (const auto cached = cache.Find(type))
    {
        return *cached;
    }

    const auto resolved = ResolveComponent(type, type_name);
    if (resolved.HasError())
    {
        return Unexpected{ resolved.Error() };
    }
    cache.Insert(type, ReadableComponent{ resolved.Value() });
    return resolved.Value();
}

/** reader의 오류를 월드 파일 읽기 오류 메시지로 만듭니다. */
[[nodiscard]] String ReaderErrorOf(const ArchiveReader& reader)
{
    return String::Format("WorldFileReader: {}", reader.GetError());
}

/**
 * 루트 struct를 열고 format_version을 확인한 뒤 entities 배열을 열어, 엔티티 수를 돌려줍니다.
 * 읽은 뒤에는 EndDocument로 닫아야 합니다.
 */
[[nodiscard]] Expected<u64, String> BeginDocument(ArchiveReader& reader)
{
    reader.BeginStruct();
    if (reader.HasError())
    {
        return Unexpected{ ReaderErrorOf(reader) };
    }

    if (!reader.Field("format_version"))
    {
        return Unexpected{ "WorldFileReader: 'format_version' is missing." };
    }
    i64 version = 0;
    reader.Int(version, EIntWidth::Bits32, true);
    if (reader.HasError())
    {
        return Unexpected{ ReaderErrorOf(reader) };
    }
    if (version != FORMAT_VERSION)
    {
        return Unexpected{ String::Format("WorldFileReader: format_version {} is not supported (expected {}).", version, FORMAT_VERSION) };
    }

    if (!reader.Field("entities"))
    {
        return Unexpected{ "WorldFileReader: the 'entities' array is missing." };
    }
    u64 count = 0;
    reader.BeginSeq(count);
    if (reader.HasError())
    {
        return Unexpected{ ReaderErrorOf(reader) };
    }
    return count;
}

/** BeginDocument가 연 entities 배열과 루트 struct를 닫습니다. */
[[nodiscard]] Expected<void, String> EndDocument(ArchiveReader& reader)
{
    reader.EndSeq();
    reader.EndStruct();
    if (reader.HasError())
    {
        return Unexpected{ ReaderErrorOf(reader) };
    }
    return {};
}

/**
 * 파일의 엔티티를 모두 만들어 out_entities에 담고 remapper에 짝지어 넣습니다. 엔티티의 id만 읽고 components는 건너뜁니다.
 * 중간에 실패해도 그때까지 만든 엔티티는 out_entities에 남습니다.
 */
[[nodiscard]] Expected<void, String> CreateEntities(ArchiveReader& reader, World& world, EntityRemapper& remapper, Array<LoadedEntity>& out_entities)
{
    const auto count = BeginDocument(reader);
    if (count.HasError())
    {
        return Unexpected{ count.Error() };
    }

    HashSet<u64> used_ids = CollectPersistentIds(world);
    for (usize index = 0; index < count.Value(); ++index)
    {
        reader.BeginStruct();
        i64 id = 0;
        const bool has_id = reader.Field("id");
        if (has_id)
        {
            reader.Int(id, EIntWidth::Bits64, false);
        }
        if (reader.Field("components"))
        {
            reader.SkipSection();
        }
        reader.EndStruct();
        if (reader.HasError())
        {
            return Unexpected{ ReaderErrorOf(reader) };
        }
        if (!has_id || id == 0)
        {
            return Unexpected{ String::Format("WorldFileReader: entities[{}] has no 'id' of a non-zero integer.", index) };
        }

        const u64 file_id = static_cast<u64>(id);
        const Entity entity = world.SpawnEntity();
        out_entities.Push(LoadedEntity{ .entity = entity, .file_id = file_id });
        if (!remapper.Add(entity, file_id))
        {
            return Unexpected{ String::Format("WorldFileReader: entity id {} appears more than once.", file_id) };
        }

        // world에 이미 있는 ID면 새 ID를 붙임. 파일 안의 참조는 remapper가 파일의 ID로 이 엔티티에 이어 줌
        const u64 persistent_id = used_ids.Insert(file_id) ? file_id : NewPersistentId(used_ids);
        world.AddComponent(entity, PersistentIdComponent{ .id = persistent_id });
    }
    return EndDocument(reader);
}

/**
 * components의 Map 엔트리 하나를 읽어 loaded.entity에 컴포넌트로 붙입니다.
 * 모르는 타입과 Transient가 붙은 타입은 건너뛰고(텍스트 포맷만 가능하고 바이너리 포맷은 오류), 건너뛴 일과 reader의 경고를 out_warnings에 남깁니다.
 */
[[nodiscard]] Expected<void, String> ReadComponentEntry(
    ArchiveReader& reader, World& world, EntityRemapper& remapper, const LoadedEntity& loaded,
    HashMap<TypeId, ReadableComponent>& cache, Array<String>& out_warnings)
{
    reader.BeginMapEntry();
    String type_name;
    reader.Str(type_name);
    if (reader.HasError())
    {
        return Unexpected{ ReaderErrorOf(reader) };
    }

    const auto component = FindOrResolveComponent(cache, TypeId::FromCanonicalName(type_name), type_name);
    if (component.HasError())
    {
        return Unexpected{ String::Format("WorldFileReader: entity {}: {}", loaded.file_id, component.Error()) };
    }

    if (component.Value().kind != ReadableComponent::EKind::Saved)
    {
        const bool is_unknown = component.Value().kind == ReadableComponent::EKind::Unknown;

        // 바이너리는 값의 길이를 알 수 없어 건너뛸 수 없음
        if (!reader.IsTextFormat())
        {
            return Unexpected{ String::Format(
                "WorldFileReader: entity {}: component type '{}' is {}, and a binary world file cannot skip it.",
                loaded.file_id, type_name, is_unknown ? "unknown" : "not saved in world files") };
        }

        out_warnings.Push(is_unknown
            ? String::Format("WorldFileReader: entity {}: unknown component type '{}' is skipped.", loaded.file_id, type_name)
            : String::Format("WorldFileReader: entity {}: component '{}' is not saved in world files and is skipped.", loaded.file_id, type_name));
        reader.EndMapEntry();
        return {};
    }

    IComponentStorage* const storage = component.Value().ops->ensure_storage(world);
    storage->EmplaceDefault(loaded.entity);

    const usize reader_warning_count = reader.GetWarnings().Len();
    const usize unresolved_count = remapper.GetUnresolvedIds().Len();
    const auto result = serde::Deserialize(reader, *component.Value().plan, storage->GetRaw(loaded.entity));

    // 필드 이름을 바꿨거나 오타를 낸 키를 알아차리게 모두 남김
    for (const String& warning : reader.GetWarnings() | std::views::drop(reader_warning_count))
    {
        out_warnings.Push(String::Format("WorldFileReader: entity {}, component '{}': {}", loaded.file_id, type_name, warning));
    }

    if (result.HasError())
    {
        return Unexpected{ String::Format(
            "WorldFileReader: entity {}: component '{}' at '{}': {}", loaded.file_id, type_name, result.Error().path, result.Error().message) };
    }
    reader.EndMapEntry();

    // 파일에 없는 엔티티를 가리키는 참조는 null이 되므로 어느 컴포넌트였는지 남김
    const ArrayView<const u64> unresolved_ids = remapper.GetUnresolvedIds();
    for (const u64 missing_id : unresolved_ids | std::views::drop(unresolved_count))
    {
        out_warnings.Push(String::Format(
            "WorldFileReader: entity {}, component '{}': entity id {} is not in the file, so the reference is set to null.",
            loaded.file_id, type_name, missing_id));
    }
    return {};
}

/**
 * Rewind한 reader에서 문서를 다시 읽으며 loaded_entities의 엔티티마다 컴포넌트를 붙입니다. 엔티티는 파일의 순서와 같게 짝짓습니다.
 * 컴포넌트 밖(루트, 엔티티)에서 reader가 남긴 경고는 접두어 없이 마지막에 out_warnings에 담습니다.
 */
[[nodiscard]] Expected<void, String> ReadComponents(
    ArchiveReader& reader, World& world, EntityRemapper& remapper, ArrayView<const LoadedEntity> loaded_entities, Array<String>& out_warnings)
{
    if (const auto count = BeginDocument(reader); count.HasError())
    {
        return Unexpected{ count.Error() };
    }

    Array<String> document_warnings;
    usize taken_warning_count = 0;
    const auto take_document_warnings = [&]
    {
        for (const String& warning : reader.GetWarnings() | std::views::drop(taken_warning_count))
        {
            document_warnings.Push(warning);
        }
        taken_warning_count = reader.GetWarnings().Len();
    };

    HashMap<TypeId, ReadableComponent> cache;
    for (const LoadedEntity& loaded : loaded_entities)
    {
        reader.BeginStruct();

        // 값은 처음 읽을 때 얻었으므로 쓰지 않고, 바이너리의 필드 순서를 맞추려고 읽음
        i64 id = 0;
        if (reader.Field("id"))
        {
            reader.Int(id, EIntWidth::Bits64, false);
        }

        if (reader.Field("components"))
        {
            reader.BeginSection();
            u64 component_count = 0;
            reader.BeginMap(component_count);
            for (u64 index = 0; index < component_count && !reader.HasError(); ++index)
            {
                take_document_warnings();
                if (const auto result = ReadComponentEntry(reader, world, remapper, loaded, cache, out_warnings); result.HasError())
                {
                    return Unexpected{ result.Error() };
                }
                taken_warning_count = reader.GetWarnings().Len();
            }
            reader.EndMap();
            reader.EndSection();
        }
        reader.EndStruct();
        if (reader.HasError())
        {
            return Unexpected{ ReaderErrorOf(reader) };
        }
    }

    if (const auto result = EndDocument(reader); result.HasError())
    {
        return Unexpected{ result.Error() };
    }
    take_document_warnings();
    for (String& warning : document_warnings)
    {
        out_warnings.Push(std::move(warning));
    }
    return {};
}
} // namespace

WorldFileWriter::WorldFileWriter(World& in_world)
    : world(in_world)
{
}

Expected<void, String> WorldFileWriter::Write(ArchiveWriter& writer)
{
    warnings.Clear();

    // 컴포넌트의 Entity 참조를 영속 ID로 바꿀 수 있게 모든 엔티티의 ID를 먼저 정함
    EntityRemapper remapper;
    HashSet<u64> used_ids;
    for (const Entity entity : world.GetAliveEntities())
    {
        u64 id = 0;
        if (const auto id_component = world.TryGetComponent<PersistentIdComponent>(entity))
        {
            id = id_component->id;
        }

        // 아직 ID가 없거나, 복제 등으로 앞의 엔티티와 ID가 겹치면 새 ID를 붙임
        if (id == 0 || !used_ids.Insert(id))
        {
            id = NewPersistentId(used_ids);
            world.AddComponent(entity, PersistentIdComponent{ .id = id });
        }

        [[maybe_unused]] const bool is_added = remapper.Add(entity, id);
        SE_ASSERT(is_added, "WorldFileWriter: entity (id {}) or persistent id {} is added twice.", entity.GetId(), id);
    }

    SerializeContext context;
    context.Add(remapper);
    writer.SetContext(&context);
    SE_SCOPE_DEFER { writer.SetContext(nullptr); };

    // 컴포넌트 타입마다 한 번만 정보를 모으고, 바이너리 포맷도 같은 출력이 되게 이름순으로 씀
    Array<SavedComponent> saved_components;
    for (const auto& [legacy_type, storage] : world.component_storages)
    {
        if (auto saved = MakeSavedComponent(legacy_type, *storage))
        {
            saved_components.Push(std::move(*saved));
        }
    }
    saved_components.SortBy(&SavedComponent::name);

    writer.BeginStruct();
    writer.Field("format_version");
    writer.Int(FORMAT_VERSION, EIntWidth::Bits32, true);
    writer.Field("entities");
    writer.BeginSeq(world.GetAliveEntities().Len(), ESeqOrder::Ordered);
    for (const Entity entity : world.GetAliveEntities())
    {
        const u64 id = world.GetComponent<PersistentIdComponent>(entity).id;

        // 바이너리 포맷은 Map의 개수를 먼저 써야 하므로 이 엔티티가 가진 컴포넌트를 먼저 모음
        Array<const SavedComponent*> present_components;
        for (const SavedComponent& saved : saved_components)
        {
            if (!saved.storage->Contains(entity))
            {
                continue;
            }
            if (!saved.error.IsEmpty())
            {
                return Unexpected{ String::Format("WorldFileWriter: entity {}: {}", id, saved.error) };
            }
            present_components.Push(&saved);
        }

        writer.BeginStruct();
        writer.Field("id");
        writer.Int(static_cast<i64>(id), EIntWidth::Bits64, false);
        writer.Field("components");
        writer.BeginSection();
        writer.BeginMap(present_components.Len());
        for (const SavedComponent* const saved : present_components)
        {
            writer.BeginMapEntry();
            writer.Str(saved->name);
            const usize unresolved_count = remapper.GetUnresolvedEntityCount();
            if (const auto result = serde::Serialize(writer, *saved->plan, saved->storage->GetRaw(entity)); result.HasError())
            {
                return Unexpected{ String::Format(
                    "WorldFileWriter: entity {}: component '{}' at '{}': {}", id, saved->name, result.Error().path, result.Error().message) };
            }
            writer.EndMapEntry();

            // 저장하지 않는 엔티티를 가리키는 참조는 null로 쓰이므로 어느 컴포넌트였는지 남김
            if (remapper.GetUnresolvedEntityCount() > unresolved_count)
            {
                warnings.Push(String::Format(
                    "WorldFileWriter: entity {}, component '{}': a reference to an entity that is not saved is written as null.", id, saved->name));
            }
        }
        writer.EndMap();
        writer.EndSection();
        writer.EndStruct();
    }
    writer.EndSeq();
    writer.EndStruct();

    if (writer.HasError())
    {
        return Unexpected{ String::Format("WorldFileWriter: {}", writer.GetError()) };
    }
    return {};
}

ArrayView<const String> WorldFileWriter::GetWarnings() const
{
    return warnings;
}

WorldFileReader::WorldFileReader(World& in_world)
    : world(in_world)
{
}

Expected<void, String> WorldFileReader::Read(ArchiveReader& reader)
{
    warnings.Clear();

    // 실패하면 이번에 만든 엔티티를 지워 world를 Read 전으로 되돌림
    Array<LoadedEntity> loaded_entities;
    SE_SCOPE_DEFER_NAMED(rollback)
    {
        for (const LoadedEntity& loaded : loaded_entities)
        {
            world.DestroyEntity(loaded.entity);
        }
    };

    EntityRemapper remapper;
    SerializeContext context;
    context.Add(remapper);
    reader.SetContext(&context);
    SE_SCOPE_DEFER { reader.SetContext(nullptr); };

    // 뒤에 나오는 엔티티를 가리키는 참조도 풀리도록 엔티티를 모두 먼저 만든 뒤, 처음부터 다시 읽으며 컴포넌트를 읽음
    if (const auto result = CreateEntities(reader, world, remapper, loaded_entities); result.HasError())
    {
        return Unexpected{ result.Error() };
    }
    reader.Rewind();
    if (const auto result = ReadComponents(reader, world, remapper, loaded_entities, warnings); result.HasError())
    {
        return Unexpected{ result.Error() };
    }

    rollback.Discard();
    return {};
}

ArrayView<const String> WorldFileReader::GetWarnings() const
{
    return warnings;
}
} // namespace se
