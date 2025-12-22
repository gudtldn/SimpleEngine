#include "Core/Types/Guid.h"

#define UUID_SYSTEM_GENERATOR
#include "stduuid/uuid.h"


namespace
{
uuids::uuid ToUuid(const se::Guid& guid)
{
    static_assert(sizeof(se::Guid) == sizeof(uuids::uuid));
    static_assert(alignof(se::Guid) == alignof(uuids::uuid));
    return std::bit_cast<uuids::uuid>(guid);
}

se::Guid FromUuid(const uuids::uuid& uuid)
{
    static_assert(sizeof(se::Guid) == sizeof(uuids::uuid));
    static_assert(alignof(se::Guid) == alignof(uuids::uuid));
    return std::bit_cast<se::Guid>(uuid);
}
}  // namespace


namespace se
{
const Guid Guid::None{};

Guid Guid::NewGuid()
{
    return FromUuid(uuids::uuid_system_generator{}());
}

Guid Guid::FromString(const String& str)
{
    return FromUuid(uuids::uuid::from_string(str.Bytes()).value_or(uuids::uuid{}));
}

bool Guid::IsValid() const noexcept
{
    uint64 parts[2];
    std::memcpy(parts, &data, sizeof(parts));
    return (parts[0] | parts[1]) != 0;
}

String Guid::ToString() const
{
    return String{ uuids::to_string(ToUuid(*this)) };
}

Guid::operator bool() const noexcept
{
    return IsValid();
}
}  // namespace se

size_t std::hash<se::Guid>::operator()(const se::Guid& guid) const noexcept
{
    return std::hash<uuids::uuid>{}(ToUuid(guid));
}
