#include "Core/Serialization/Archive.h"


namespace se::core
{
#define IMPL_PROCESS_PRIMITIVE(type) \
    void Archive::Process(type& value, const char* name) \
    { \
        ProcessRaw(&value, sizeof(type), name); \
    }

IMPL_PROCESS_PRIMITIVE(int8)
IMPL_PROCESS_PRIMITIVE(uint8)
IMPL_PROCESS_PRIMITIVE(int16)
IMPL_PROCESS_PRIMITIVE(uint16)
IMPL_PROCESS_PRIMITIVE(int32)
IMPL_PROCESS_PRIMITIVE(uint32)
IMPL_PROCESS_PRIMITIVE(int64)
IMPL_PROCESS_PRIMITIVE(uint64)
IMPL_PROCESS_PRIMITIVE(float)
IMPL_PROCESS_PRIMITIVE(double)

#undef IMPL_PROCESS_PRIMITIVE


void Archive::Process(bool& value, const char* name)
{
    uint8 bool_value = value ? 1 : 0;
    ProcessRaw(&bool_value, sizeof(uint8), name);

    if (IsLoading())
    {
        value = bool_value != 0;
    }
}

void Archive::Process(String& value, const char* name)
{
    usize length;
    if (IsSaving())
    {
        length = value.ByteLen();
    }
    Process(length, nullptr);

    if (IsLoading())
    {
        value.ResizeForOverwrite(length);
    }
    ProcessRaw(value.Data(), length, name);
}

void Archive::Process(StringName& value, const char* name)
{
    String name_str;
    if (IsSaving())
    {
        name_str = value.ToString();
    }

    Process(name_str, name);

    if (IsLoading())
    {
        value = name_str;
    }
}

void Archive::Process(Guid& value, const char* name)
{
    ProcessRaw(&value, sizeof(Guid), name);
}
}
