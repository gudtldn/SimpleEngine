#include "SimpleEngine/ECS/World.h"

#include "SimpleEngine/Core/Logging/Logging.h"
#include "SimpleEngine/Core/Reflection/Meta.h"
#include "SimpleEngine/Core/Reflection/TypeRegistry.h"
#include "SimpleEngine/ECS/ComponentStorage.h"
#include "SimpleEngine/ECS/ECSRegistry.h"

#include <ranges>


namespace se
{
namespace
{
/**
 * 직렬화 대상 여부를 판정합니다.
 * Transient이거나 serialize 콜백이 없으면 제외
 */
[[nodiscard]] bool ShouldSerializeType(const TypeInfo& info)
{
    if (info.flags.IsSet(ETypeFlags::Transient))
    {
        return false;
    }
    if (info.serialize == nullptr)
    {
        return false;
    }
    return true;
}
} // namespace

void World::Reset()
{
    component_storages.Clear();
    alive_entities.Clear();
    entity_manager.Reset();
    active_command_buffer = nullptr;
}

void World::DestroyEntity(Entity entity)
{
    for (const auto& storage : component_storages | std::views::values)
    {
        storage->Remove(entity);
    }

    // TODO: 여기 최적화의 여지가 있음. 지금 선형 탐색중, 추후 FlatSet이나 이진탐색으로 최적화 가능
    if (const Optional<usize> idx = alive_entities.Find(entity))
    {
        alive_entities.RemoveAtSwap(*idx);
    }

    entity_manager.Destroy(entity);
}

IComponentStorage* World::FindRawStorage(const TypeId& type_id)
{
    if (const auto storage = component_storages.Find(type_id))
    {
        return storage->get();
    }

    return nullptr;
}

const IComponentStorage* World::FindRawStorage(const TypeId& type_id) const
{
    if (const auto storage = component_storages.Find(type_id))
    {
        return storage->get();
    }

    return nullptr;
}

IComponentStorage* World::GetOrCreateRawStorage(const TypeId& type_id)
{
    if (const auto storage = component_storages.Find(type_id))
    {
        return storage->get();
    }

    if (const auto ops = ECSRegistry::Get().GetComponentOps(type_id))
    {
        return ops->ensure_storage(*this);
    }

    return nullptr;
}

void Serialize(Archive& ar, World& world)
{
    // --- Header ---
    u32 magic = World::FILE_MAGIC;
    u32 version = World::FILE_VERSION;
    ar("magic") << magic;
    ar("version") << version;

    if (ar.IsLoading())
    {
        if (magic != World::FILE_MAGIC)
        {
            ar.SetError("Invalid world file magic");
            return;
        }
        if (version != World::FILE_VERSION)
        {
            ar.SetError("Unsupported world file version");
            return;
        }

        // Entity/Component 초기화 (Resource는 유지)
        world.Reset();
    }

    // --- EntityManager ---
    ar("entity_manager") << world.entity_manager;

    // --- Alive Entities ---
    ar("alive_entities") << world.alive_entities;

    // --- Components ---
    const TypeRegistry& registry = TypeRegistry::Get();
    const ECSRegistry& ecs_registry = ECSRegistry::Get();

    if (ar.IsSaving())
    {
        // 직렬화 대상 storage 수 카운트
        u64 type_count = 0;
        for (const auto& [type_id, storage] : world.component_storages)
        {
            if (storage->IsEmpty())
            {
                continue;
            }

            if (const auto info = registry.Find(type_id))
            {
                if (ShouldSerializeType(*info))
                {
                    ++type_count;
                }
            }
        }

        ar("components");
        ar.BeginMap(type_count);
        for (auto& [type_id, storage] : world.component_storages)
        {
            if (storage->IsEmpty())
            {
                continue;
            }

            auto info_opt = registry.Find(type_id);
            if (!info_opt.HasValue() || !ShouldSerializeType(*info_opt))
            {
                continue;
            }

            ar.BeginMapKey();
            ar << type_id;
            ar.EndMapKey();

            ar.BeginMapValue();
            u64 entity_count = storage->Len();
            ar.BeginArray(entity_count);
            for (u64 i = 0; i < entity_count; ++i)
            {
                Entity entity = *storage->GetEntityByIndex(static_cast<usize>(i));
                void* raw = storage->GetRaw(entity);

                ar.BeginObject();
                ar("entity") << entity;
                ar("data");
                info_opt->serialize(ar, raw);
                ar.EndObject();
            }
            ar.EndArray();
            ar.EndMapValue();
        }
        ar.EndMap();
    }
    else // Loading
    {
        u64 type_count = 0;
        ar("components");
        ar.BeginMap(type_count);
        for (u64 t = 0; t < type_count; ++t)
        {
            ar.BeginMapKey();
            TypeId type_id;
            ar << type_id;
            ar.EndMapKey();

            ar.BeginMapValue();

            const auto info_opt = registry.Find(type_id);
            const auto ops_opt = ecs_registry.GetComponentOps(type_id);

            if (!info_opt.HasValue() || !ops_opt.HasValue())
            {
                if (ar.IsBinary())
                {
                    // TODO: 컴포넌트 블록에 byte_size를 저장하면 binary에서도 skip 가능
                    // Binary: data block의 byte size를 모르므로 skip 불가 -> fatal
                    ar.SetError("Unknown component type in binary archive (forward-skip not supported)");
                    return;
                }

                // Text: key 기반이므로 미소비 데이터는 자동 무시됨
                ConsoleLog(ELogLevel::Warning, "World deserialization: unknown component type '{}', skipping.", type_id.GetName());
                u64 entity_count = 0;
                ar.BeginArray(entity_count);
                for (u64 i = 0; i < entity_count; ++i)
                {
                    ar.BeginObject();
                    ar.EndObject();
                }
                ar.EndArray();
                ar.EndMapValue();
                continue;
            }

            u64 entity_count = 0;
            IComponentStorage* storage = ops_opt->ensure_storage(world);

            ar.BeginArray(entity_count);
            for (u64 i = 0; i < entity_count; ++i)
            {
                ar.BeginObject();

                Entity entity;
                ar("entity") << entity;

                // 기본 생성한 뒤 raw ptr에 역직렬화로 덮어쓰기
                storage->EmplaceDefault(entity);
                void* raw = storage->GetRaw(entity);
                ar("data");
                info_opt->serialize(ar, raw);

                ar.EndObject();
            }
            ar.EndArray();
            ar.EndMapValue();
        }
        ar.EndMap();
    }
}
} // namespace se
