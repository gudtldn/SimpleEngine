#include "Core/Serialization/Archive.h"

#include "Core/Container/String.h"
#include "Core/Types/Guid.h"
#include "Core/Types/StringName.h"


namespace se::core
{
#define IMPL_ARCHIVE_OPERATOR(Type) \
    Archive& Archive::operator<<(Type& value) \
    { \
        ProcessBytes(&value, sizeof(Type)); \
        return *this; \
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
}
