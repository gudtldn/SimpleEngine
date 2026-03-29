#pragma once

#include "SimpleEngine/ECS/IStorage.h"
#include "SimpleEngine/ECS/SparseSet.h"
#include "SimpleEngine/Traits/TypeTraits.h"

#include <type_traits>


namespace se
{
/** IStorage 인터페이스와 SparseSet<T>를 연결하는 타입별 컴포넌트 저장소입니다. */
template <typename ComponentType>
class ComponentStorage : public IStorage
{
    SparseSet<ComponentType> storage;

public:
    ComponentStorage() = default;

    //~Begin IStorage
    [[nodiscard]] virtual usize Len() const noexcept override
    {
        return storage.Len();
    }

    [[nodiscard]] virtual bool IsEmpty() const noexcept override
    {
        return storage.IsEmpty();
    }

    [[nodiscard]] virtual bool Contains(Entity entity) const noexcept override
    {
        return storage.Contains(entity);
    }

    [[nodiscard]] virtual Optional<Entity> GetEntityByIndex(usize index) const override
    {
        return storage.GetEntityByIndex(index);
    }

    virtual void EmplaceDefault(Entity entity) override
    {
        static_assert(std::is_default_constructible_v<ComponentType>, "ComponentType must be default constructible");
        storage.Add(entity, ComponentType{});
    }

    virtual void Remove(Entity entity) override
    {
        storage.Remove(entity);
    }

    [[nodiscard]] virtual void* GetRaw(Entity entity) override
    {
        if (auto opt = storage.Find(entity))
        {
            return std::addressof(*opt);
        }
        return nullptr;
    }

    [[nodiscard]] virtual const void* GetRaw(Entity entity) const override
    {
        if (auto opt = storage.Find(entity))
        {
            return std::addressof(*opt);
        }
        return nullptr;
    }
    //~End IStorage

    template <typename Self>
    [[nodiscard]] traits::CopyConst<Self, SparseSet<ComponentType>&> GetStorage(this Self&& self)
    {
        return self.storage;
    }
};
} // namespace se
