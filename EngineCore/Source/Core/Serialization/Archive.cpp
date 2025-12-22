#include "Core/Serialization/Archive.h"

#include "Core/Container/String.h"
#include "Core/Types/Guid.h"
#include "Core/Types/StringName.h"
#include "Reflection/TypeId.h"
#include "Utility/Debug.h"


namespace se::core
{
#define IMPL_ARCHIVE_OPERATOR(Type) \
    Archive& Archive::operator<<(Type& value) \
    { \
        ProcessBytes(&value, sizeof(Type)); \
        return *this; \
    }

Archive& Archive::operator<<(Archive&)
{
    return *this;
}

IMPL_ARCHIVE_OPERATOR(int8)
IMPL_ARCHIVE_OPERATOR(uint8)
IMPL_ARCHIVE_OPERATOR(int16)
IMPL_ARCHIVE_OPERATOR(uint16)
IMPL_ARCHIVE_OPERATOR(int32)
IMPL_ARCHIVE_OPERATOR(uint32)
IMPL_ARCHIVE_OPERATOR(int64)
IMPL_ARCHIVE_OPERATOR(uint64)
IMPL_ARCHIVE_OPERATOR(float)
IMPL_ARCHIVE_OPERATOR(double)

#undef IMPL_ARCHIVE_OPERATOR


Archive& Archive::operator<<(bool& value)
{
    uint8 bool_value = value ? 1 : 0;
    *this << bool_value;

    if (IsLoading())
    {
        value = bool_value != 0;
    }
    return *this;
}

Archive& Archive::operator<<(String& value)
{
    uint64 length;
    if (IsSaving())
    {
        length = value.ByteLen();
    }
    *this << length;

    if (IsLoading())
    {
        value.ResizeForOverwrite(length);
    }
    ProcessBytes(value.Data(), length);
    return *this;
}

Archive& Archive::operator<<(StringName& value)
{
    String name_str;
    if (IsSaving())
    {
        name_str = value.ToString();
    }

    *this << name_str;

    if (IsLoading())
    {
        value = name_str;
    }

    return *this;
}

Archive& Archive::operator<<(Guid& value)
{
    ProcessBytes(&value, sizeof(Guid));
    return *this;
}

Archive& Archive::operator<<(refl::TypeId& value)
{
    if (IsBinary())
    {
        uint64 hash = 0;
        if (IsSaving())
        {
            if (SE_ENSURE(value.IsValid(), "Attempting to save invalid TypeId via Binary!"))
            {
                hash = value.GetHash();
            }
        }

        *this << hash;

        if (IsLoading())
        {
            value = refl::TypeId::FromHash(hash);
            if (!value.IsValid())
            {
                ConsoleLog(ELogLevel::Error, "Failed to resolve TypeId from hash: {}. The class might be deleted or renamed.", hash);
            }
        }
    }
    else
    {
        String type_name;
        if (IsSaving())
        {
            if (SE_ENSURE(value.IsValid(), "Attempting to save invalid TypeId via Text!"))
            {
                type_name = value.GetName();
            }
        }

        *this << type_name;

        if (IsLoading())
        {
            value = refl::TypeId::FromName(type_name);
            if (!value.IsValid())
            {
                ConsoleLog(ELogLevel::Error, "Failed to resolve TypeId from name: '{}'. The class might be deleted or renamed.", type_name);
                SE_BREAKPOINT();
            }
        }
    }
    return *this;
}
}  // namespace se::core
