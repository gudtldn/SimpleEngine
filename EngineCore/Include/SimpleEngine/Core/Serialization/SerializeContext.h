#pragma once

#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Reflection/TypeId.h"
#include "SimpleEngine/Core/Reflection/TypeName.h"
#include "SimpleEngine/Utility/Debug.h"


namespace se
{
/**
 * 직렬화 작업 하나 동안 트레이트가 쓸 서비스(Entity 리맵 등)를 타입으로 찾게 해 줍니다.
 * 작업마다 새로 만들어 archive의 SetContext로 넘기고, 스레드 간에 공유하지 않습니다.
 * 서비스를 소유하지 않으므로, 넣은 서비스는 작업이 끝날 때까지 살아 있어야 합니다.
 */
class SerializeContext
{
public:
    /** service를 Service 타입으로 넣습니다. 같은 타입은 한 번만 넣을 수 있습니다. */
    template <typename Service>
    void Add(Service& service)
    {
        constexpr TypeId id = TypeId::Of<Service>();
        SE_ASSERT(!services.Contains(id), "SerializeContext: a service of type '{}' is already added.", TypeNameOf<Service>());
        services.Insert(id, &service);
    }

    /** Service 타입으로 넣은 서비스를 찾습니다. 없으면 nullptr를 돌려줍니다. */
    template <typename Service>
    [[nodiscard]] Service* Find() const
    {
        if (const auto service = services.Find(TypeId::Of<Service>()))
        {
            return static_cast<Service*>(*service);
        }
        return nullptr;
    }

private:
    HashMap<TypeId, void*> services;
};
} // namespace se
