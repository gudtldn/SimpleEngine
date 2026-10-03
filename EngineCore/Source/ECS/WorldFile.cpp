#include "SimpleEngine/ECS/WorldFile.h"

#include "SimpleEngine/Core/Container/HashSet.h"
#include "SimpleEngine/Core/Math/Random.h"
#include "../../Include/SimpleEngine/Core/Reflection/Legacy/TypeRegistry.h"
#include "SimpleEngine/Core/Reflection/TypeRegistry.h"
#include "SimpleEngine/Core/Serialization/SerializeContext.h"
#include "SimpleEngine/Core/Serialization/SerializePlanRegistry.h"
#include "SimpleEngine/Core/Serialization/Serializer.h"
#include "SimpleEngine/Core/Serialization/TomlArchive.h"
#include "SimpleEngine/Core/Serialization/Transient.h"
#include "SimpleEngine/ECS/Components/PersistentIdComponent.h"
#include "SimpleEngine/ECS/ECSRegistry.h"
#include "SimpleEngine/ECS/EntityRemapper.h"
#include "SimpleEngine/ECS/World.h"
#include "SimpleEngine/Utility/Common.h"

#include <algorithm>
#include <ranges>
#include <sstream>
#include <string_view>


namespace se
{
namespace
{
/** 월드 파일 문서 구조(루트, 엔티티 테이블)의 버전 */
constexpr i64 FORMAT_VERSION = 1;

/** 레거시 직렬화가 쓴 바이너리 월드 파일의 첫 4바이트 */
constexpr StringView LEGACY_BINARY_MAGIC = "SEWD";

/** 영속 ID를 TOML 정수(i64)로 쓸 수 있게 하위 63비트만 남기는 마스크 */
constexpr u64 PERSISTENT_ID_MASK = (u64{ 1 } << 63) - 1;

/** Read가 파일의 엔티티 테이블 하나로 만든 엔티티 */
struct LoadedEntity
{
    Entity entity;

    /** 파일에 적힌 영속 ID. 파일 안의 Entity 참조는 이 ID로 엔티티를 가리킵니다. */
    u64 file_id = 0;

    const toml::table* table = nullptr;
};

/** used_ids에 없는 영속 ID를 무작위로 만들어 used_ids에 넣고 돌려줍니다. 범위는 [1, 2^63)입니다. */
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
 * node 아래에서 스칼라 값만 담은 테이블을 인라인 테이블로 표시해 한 줄로 쓰이게 합니다.
 * 예: position = { x = 1.0, y = 2.0, z = 3.0 }
 */
void MarkScalarTablesInline(toml::node& node)
{
    if (toml::array* const array = node.as_array())
    {
        for (toml::node& element : *array)
        {
            MarkScalarTablesInline(element);
        }
        return;
    }

    toml::table* const table = node.as_table();
    if (table == nullptr)
    {
        return;
    }

    bool has_only_scalars = true;
    for (auto&& entry : *table)
    {
        MarkScalarTablesInline(entry.second);
        has_only_scalars = has_only_scalars && !entry.second.is_table() && !entry.second.is_array();
    }
    table->is_inline(has_only_scalars);
}

/**
 * entity가 가진 컴포넌트 하나를 타입 이름을 키로 out_components에 씁니다. Transient가 붙은 타입은 쓰지 않습니다.
 * 등록되지 않은 타입이라 쓸 수 없으면 오류 메시지를 돌려줍니다.
 */
[[nodiscard]] Expected<void, String> WriteComponent(
    const TypeId_v1& legacy_type, const IComponentStorage& storage, Entity entity, SerializeContext& context, toml::table& out_components)
{
    const auto ops = ECSRegistry::Get().GetComponentOps(legacy_type);
    if (!ops)
    {
        return Unexpected{ String::Format("component '{}' is not registered as an ECS component (meta::Component).", LegacyTypeNameOf(legacy_type)) };
    }

    // 등록이 없는 컴포넌트를 건너뛰면 파일에서 조용히 사라지므로 오류로 처리
    const auto info = TypeRegistry::Get().Find(ops->type);
    if (!info)
    {
        return Unexpected{ String::Format(
            "component '{}' is not registered with SE_REFLECT_BEGIN. Register its fields, or annotate it with Transient if it must not be saved.",
            LegacyTypeNameOf(legacy_type)) };
    }
    if (info->HasAnnotation<TransientAnnotation>())
    {
        return {};
    }

    const auto plan = SerializePlanRegistry::Get().FindOrCompile(ops->type);
    if (!plan)
    {
        return Unexpected{ String::Format("component '{}': {}", info->name, plan.Error()) };
    }

    toml::table component_table;
    TomlWriter writer(component_table);
    writer.SetContext(&context);
    if (const auto result = serde::Serialize(writer, *plan.Value(), storage.GetRaw(entity)); result.HasError())
    {
        return Unexpected{ String::Format("component '{}' at '{}': {}", info->name, result.Error().path, result.Error().message) };
    }

    for (auto&& field : component_table)
    {
        MarkScalarTablesInline(field.second);
    }
    out_components.insert(std::string_view{ info->name }, std::move(component_table));
    return {};
}

/**
 * 파일의 컴포넌트 테이블 하나를 읽어 loaded.entity에 붙입니다.
 * 모르는 타입과 Transient가 붙은 타입은 건너뛰고, 건너뛴 일과 TomlReader의 경고를 out_warnings에 남깁니다.
 */
[[nodiscard]] Expected<void, String> ReadComponent(
    World& world, const LoadedEntity& loaded, StringView type_name, const toml::node& node, SerializeContext& context, Array<String>& out_warnings)
{
    const TypeId type = TypeId::FromCanonicalName(type_name);
    const auto info = TypeRegistry::Get().Find(type);
    const auto ops = FindComponentOps(type);
    if (!info || !ops)
    {
        out_warnings.Push(String::Format("WorldFileReader: entity {}: unknown component type '{}' is skipped.", loaded.file_id, type_name));
        return {};
    }
    if (info->HasAnnotation<TransientAnnotation>())
    {
        out_warnings.Push(String::Format("WorldFileReader: entity {}: component '{}' is not saved in world files and is skipped.", loaded.file_id, type_name));
        return {};
    }

    const toml::table* const component_table = node.as_table();
    if (component_table == nullptr)
    {
        return Unexpected{ String::Format("component '{}' is not a table.", type_name) };
    }

    const auto plan = SerializePlanRegistry::Get().FindOrCompile(type);
    if (!plan)
    {
        return Unexpected{ String::Format("component '{}': {}", type_name, plan.Error()) };
    }

    IComponentStorage* const storage = ops->ensure_storage(world);
    storage->EmplaceDefault(loaded.entity);

    TomlReader reader(*component_table);
    reader.SetContext(&context);
    const auto result = serde::Deserialize(reader, *plan.Value(), storage->GetRaw(loaded.entity));

    // 필드 이름을 바꿨거나 오타를 낸 키를 알아차리게 모두 남김
    for (const String& warning : reader.GetWarnings())
    {
        out_warnings.Push(String::Format("WorldFileReader: entity {}, component '{}': {}", loaded.file_id, type_name, warning));
    }

    if (result.HasError())
    {
        return Unexpected{ String::Format("component '{}' at '{}': {}", type_name, result.Error().path, result.Error().message) };
    }
    return {};
}
} // namespace

WorldFileWriter::WorldFileWriter(World& in_world)
    : world(in_world)
{
}

Expected<String, String> WorldFileWriter::Write()
{
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

    toml::array entity_tables;
    for (const Entity entity : world.GetAliveEntities())
    {
        const u64 id = world.GetComponent<PersistentIdComponent>(entity).id;

        toml::table components;
        for (const auto& [type_id, storage] : world.component_storages)
        {
            if (!storage->Contains(entity))
            {
                continue;
            }
            if (const auto result = WriteComponent(type_id, *storage, entity, context, components); result.HasError())
            {
                return Unexpected{ String::Format("WorldFileWriter: entity {}: {}", id, result.Error()) };
            }
        }

        toml::table entity_table;
        entity_table.insert("id", static_cast<i64>(id));
        entity_table.insert("components", std::move(components));
        entity_tables.push_back(std::move(entity_table));
    }

    toml::table root;
    root.insert("format_version", FORMAT_VERSION);
    root.insert("entities", std::move(entity_tables));

    std::ostringstream stream;
    stream << root;
    return String{ stream.view() };
}

WorldFileReader::WorldFileReader(World& in_world)
    : world(in_world)
{
}

Expected<void, String> WorldFileReader::Read(StringView text)
{
    warnings.Clear();

    // TOML 파서의 알아보기 어려운 오류 대신 레거시 바이너리 파일임을 알림
    if (text.StartsWith(LEGACY_BINARY_MAGIC))
    {
        return Unexpected{ "WorldFileReader: this is a binary world file written by the legacy serializer, which is no longer supported." };
    }

    const toml::parse_result parsed = toml::parse(std::string_view{ text });
    if (!parsed)
    {
        const toml::source_position position = parsed.error().source().begin;
        return Unexpected{ String::Format(
            "WorldFileReader: failed to parse TOML at line {}, column {}: {}",
            position.line, position.column, parsed.error().description()) };
    }
    const toml::table& root = parsed.table();

    const auto* const version = root.get_as<i64>("format_version");
    if (version == nullptr)
    {
        return Unexpected{ "WorldFileReader: 'format_version' is missing. World files written by the legacy serializer are not supported." };
    }
    if (version->get() != FORMAT_VERSION)
    {
        return Unexpected{ String::Format("WorldFileReader: format_version {} is not supported (expected {}).", version->get(), FORMAT_VERSION) };
    }

    const auto* const entity_nodes = root.get_as<toml::array>("entities");
    if (entity_nodes == nullptr)
    {
        return Unexpected{ "WorldFileReader: the 'entities' array is missing." };
    }

    // 실패하면 이번에 만든 엔티티를 지워 world를 Read 전으로 되돌림
    Array<LoadedEntity> loaded_entities;
    SE_SCOPE_DEFER_NAMED(rollback)
    {
        for (const LoadedEntity& loaded : loaded_entities)
        {
            world.DestroyEntity(loaded.entity);
        }
    };

    // 뒤에 나오는 엔티티를 가리키는 참조도 풀리도록 엔티티를 모두 먼저 만듦
    EntityRemapper remapper;
    HashSet<u64> used_ids = CollectPersistentIds(world);
    for (const auto [index, node] : std::views::enumerate(*entity_nodes))
    {
        const toml::table* const entity_table = node.as_table();
        const auto* const id = entity_table != nullptr ? entity_table->get_as<i64>("id") : nullptr;
        if (id == nullptr || id->get() <= 0)
        {
            return Unexpected{ String::Format("WorldFileReader: entities[{}] has no 'id' of a positive integer.", index) };
        }

        const u64 file_id = static_cast<u64>(id->get());
        const Entity entity = world.SpawnEntity();
        loaded_entities.Push(LoadedEntity{ .entity = entity, .file_id = file_id, .table = entity_table });
        if (!remapper.Add(entity, file_id))
        {
            return Unexpected{ String::Format("WorldFileReader: entity id {} appears more than once.", file_id) };
        }

        // world에 이미 있는 ID면 새 ID를 붙임. 파일 안의 참조는 remapper가 파일의 ID로 이 엔티티에 이어 줌
        const u64 persistent_id = used_ids.Insert(file_id) ? file_id : NewPersistentId(used_ids);
        world.AddComponent(entity, PersistentIdComponent{ .id = persistent_id });
    }

    SerializeContext context;
    context.Add(remapper);
    for (const LoadedEntity& loaded : loaded_entities)
    {
        const toml::node* const components_node = loaded.table->get("components");
        if (components_node == nullptr)
        {
            continue;
        }
        const toml::table* const components = components_node->as_table();
        if (components == nullptr)
        {
            return Unexpected{ String::Format("WorldFileReader: entity {}: 'components' is not a table.", loaded.file_id) };
        }

        for (const auto& entry : *components)
        {
            const StringView type_name = entry.first.str();
            const usize unresolved_count = remapper.GetUnresolvedIds().Len();
            if (const auto result = ReadComponent(world, loaded, type_name, entry.second, context, warnings); result.HasError())
            {
                return Unexpected{ String::Format("WorldFileReader: entity {}: {}", loaded.file_id, result.Error()) };
            }

            // 파일에 없는 엔티티를 가리키는 참조는 null이 되므로 어느 컴포넌트였는지 남김
            const ArrayView<const u64> unresolved_ids = remapper.GetUnresolvedIds();
            for (const u64 missing_id : unresolved_ids | std::views::drop(unresolved_count))
            {
                warnings.Push(String::Format(
                    "WorldFileReader: entity {}, component '{}': entity id {} is not in the file, so the reference is set to null.",
                    loaded.file_id, type_name, missing_id));
            }
        }
    }

    rollback.Discard();
    return {};
}

ArrayView<const String> WorldFileReader::GetWarnings() const
{
    return warnings;
}
} // namespace se
