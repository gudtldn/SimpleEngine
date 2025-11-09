#include "Core/Types/Guid.h"

#define UUID_SYSTEM_GENERATOR
#include "stduuid/uuid.h"


namespace
{
const uuids::uuid& AsUuid(const Guid& guid) noexcept
{
    static_assert(sizeof(Guid) == sizeof(uuids::uuid));
    static_assert(alignof(Guid) == alignof(uuids::uuid));
    return reinterpret_cast<const uuids::uuid&>(guid);
}

uuids::uuid& AsUuid(Guid& guid) noexcept
{
    static_assert(sizeof(Guid) == sizeof(uuids::uuid));
    static_assert(alignof(Guid) == alignof(uuids::uuid));
    return reinterpret_cast<uuids::uuid&>(guid);
}

Guid FromUuid(const uuids::uuid& uuid)
{
    Guid guid;
    AsUuid(guid) = uuid;
    return guid;
}
}


Guid Guid::None{};

Guid::Guid() noexcept
{
    new(&AsUuid(*this)) uuids::uuid{};
}

Guid::~Guid()
{
    AsUuid(*this).~uuid();
}

Guid::Guid(const Guid& other) noexcept
{
    new(&AsUuid(*this)) uuids::uuid{ AsUuid(other) };
}

Guid& Guid::operator=(const Guid& other) noexcept
{
    if (this != &other)
    {
        AsUuid(*this) = AsUuid(other);
    }
    return *this;
}

Guid::Guid(Guid&& other) noexcept
{
    new(&AsUuid(*this)) uuids::uuid{ AsUuid(other) };
}

Guid& Guid::operator=(Guid&& other) noexcept
{
    if (this != &other)
    {
        AsUuid(*this) = AsUuid(other);
    }
    return *this;
}

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
    return !AsUuid(*this).is_nil();
}

se::String Guid::ToString() const
{
    return se::String{ uuids::to_string(AsUuid(*this)) };
}

Guid::operator bool() const noexcept
{
    return IsValid();
}

size_t std::hash<Guid>::operator()(const Guid& guid) const noexcept
{
    return std::hash<uuids::uuid>{}(AsUuid(guid));
}
