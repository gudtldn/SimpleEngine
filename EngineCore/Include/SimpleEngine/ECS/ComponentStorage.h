#pragma once

#include "SimpleEngine/ECS/SparseSet.h"
#include "SimpleEngine/Traits/TypeTraits.h"

#include <type_traits>


namespace se
{
/**
 * 컴포넌트 저장소의 타입 소거(type erasure)를 위한 인터페이스
 */
class SE_CORE_API IComponentStorage
{
public:
    virtual ~IComponentStorage() = default;

    /** 저장소의 엔티티 개수를 반환합니다. */
    [[nodiscard]] virtual usize Len() const noexcept = 0;

    /** 저장소가 비어있는지 확인합니다. */
    [[nodiscard]] virtual bool IsEmpty() const noexcept = 0;

    /** 해당 엔티티가 컴포넌트를 가지고 있는지 확인합니다. */
    [[nodiscard]] virtual bool Contains(Entity entity) const noexcept = 0;

    /** Dense 배열의 인덱스로 Entity를 가져옵니다. */
    [[nodiscard]] virtual Optional<Entity> GetEntityByIndex(usize index) const = 0;

    /** 주어진 엔티티에 기본 생성자로 만들어진 컴포넌트를 추가합니다. */
    virtual void EmplaceDefault(Entity entity) = 0;

    /** 엔티티가 가지고 있는 컴포넌트를 제거합니다. */
    virtual void Remove(Entity entity) = 0;

    /** 컴포넌트 데이터의 Raw Pointer를 반환합니다. (리플렉션용) */
    [[nodiscard]] virtual void* GetRaw(Entity entity) = 0;
    [[nodiscard]] virtual const void* GetRaw(Entity entity) const = 0;
};

/** IComponentStorage 인터페이스와 SparseSet<T>를 연결하는 타입별 컴포넌트 저장소 */
template <typename ComponentType>
class ComponentStorage : public IComponentStorage
{
    SparseSet<ComponentType> storage;

public:
    ComponentStorage() = default;

    //~Begin IComponentStorage
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
    //~End IComponentStorage

    template <typename Self>
    [[nodiscard]] traits::CopyConst<Self, SparseSet<ComponentType>&> GetStorage(this Self&& self)
    {
        return self.storage;
    }
};
} // namespace se
