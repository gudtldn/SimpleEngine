#include "Core/Types/Guid.h"

#define UUID_SYSTEM_GENERATOR
#include "stduuid/uuid.h"


namespace
{
uuids::uuid ToUuid(const Guid& guid)
{
    static_assert(sizeof(Guid) == sizeof(uuids::uuid));
    static_assert(alignof(Guid) == alignof(uuids::uuid));
    return std::bit_cast<uuids::uuid>(guid);
}

Guid FromUuid(const uuids::uuid& uuid)
{
    static_assert(sizeof(Guid) == sizeof(uuids::uuid));
    static_assert(alignof(Guid) == alignof(uuids::uuid));
    return std::bit_cast<Guid>(uuid);
}
}


const Guid Guid::None{};

Guid Guid::NewGuid()
{
    return FromUuid(uuids::uuid_system_generator{}());
}

Guid Guid::FromString(const se::String& str)
{
    return FromUuid(uuids::uuid::from_string(str.Bytes()).value_or(uuids::uuid{}));
}

bool Guid::IsValid() const noexcept
{
    uint64 parts[2];
    std::memcpy(parts, &data, sizeof(parts));
    return (parts[0] | parts[1]) != 0;
}

se::String Guid::ToString() const
{
    return se::String{ uuids::to_string(ToUuid(*this)) };
}

Guid::operator bool() const noexcept
{
    return IsValid();
}

size_t std::hash<Guid>::operator()(const Guid& guid) const noexcept
{
    return std::hash<uuids::uuid>{}(ToUuid(guid));
}
