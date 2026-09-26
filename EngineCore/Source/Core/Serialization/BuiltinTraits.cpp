#include "SimpleEngine/Core/Serialization/BuiltinTraits.h"

#include "SimpleEngine/Core/Serialization/SerializeOpsRegistry.h"


namespace se
{
// String
void SerializeTraits<String>::Write(ArchiveWriter& writer, const String& value)
{
    writer.Str(value);
}

void SerializeTraits<String>::Read(ArchiveReader& reader, String& value)
{
    reader.Str(value);
}


// StringName
void SerializeTraits<StringName>::Write(ArchiveWriter& writer, const StringName& value)
{
    writer.Str(value.ToString());
}

void SerializeTraits<StringName>::Read(ArchiveReader& reader, StringName& value)
{
    String temp;
    reader.Str(temp);
    if (reader.HasError())
    {
        return;
    }
    value = StringName(temp);
}


// Guid
void SerializeTraits<Guid>::Write(ArchiveWriter& writer, const Guid& value)
{
    if (writer.IsTextFormat())
    {
        writer.Str(value.ToString());
        return;
    }
    writer.Bytes(&value, sizeof(Guid));
}

void SerializeTraits<Guid>::Read(ArchiveReader& reader, Guid& value)
{
    if (reader.IsTextFormat())
    {
        String text;
        reader.Str(text);
        if (reader.HasError())
        {
            return;
        }

        const auto parsed = Guid::TryFromString(text);
        if (!parsed.HasValue())
        {
            reader.SetError(String::Format("SerializeTraits<Guid>: invalid guid string '{}'.", text));
            return;
        }
        value = parsed.Value();
        return;
    }
    reader.Bytes(&value, sizeof(Guid));
}


// TypeId
void SerializeTraits<TypeId>::Write(ArchiveWriter& writer, const TypeId& value)
{
    if (writer.IsTextFormat())
    {
        const auto info = TypeRegistry::Get().Find(value);
        if (!info.HasValue())
        {
            writer.SetError(String::Format("SerializeTraits<TypeId>: type {} is not registered.", value.Value()));
            return;
        }
        writer.Str(info.Value().name);
        return;
    }
    writer.Int(static_cast<i64>(value.Value()), EIntWidth::Bits64, false);
}

void SerializeTraits<TypeId>::Read(ArchiveReader& reader, TypeId& value)
{
    if (reader.IsTextFormat())
    {
        String name;
        reader.Str(name);
        if (reader.HasError())
        {
            return;
        }

        const TypeId candidate = TypeId::FromCanonicalName(name);
        const auto info_opt = TypeRegistry::Get().Find(candidate);
        if (!info_opt.HasValue() || info_opt->name != name)
        {
            reader.SetError(String::Format("SerializeTraits<TypeId>: unknown type name '{}'.", name));
            return;
        }
        value = candidate;
        return;
    }

    i64 packed = 0;
    reader.Int(packed, EIntWidth::Bits64, false);
    if (reader.HasError())
    {
        return;
    }
    value = TypeId::FromRaw(static_cast<u64>(packed));
}


// Path
void SerializeTraits<Path>::Write(ArchiveWriter& writer, const Path& value)
{
    writer.Str(value.ToString());
}

void SerializeTraits<Path>::Read(ArchiveReader& reader, Path& value)
{
    String temp;
    reader.Str(temp);
    if (reader.HasError())
    {
        return;
    }
    value = Path(temp);
}


// VPath
void SerializeTraits<VPath>::Write(ArchiveWriter& writer, const VPath& value)
{
    writer.Str(value.ToString());
}

void SerializeTraits<VPath>::Read(ArchiveReader& reader, VPath& value)
{
    String temp;
    reader.Str(temp);
    if (reader.HasError())
    {
        return;
    }
    value = VPath(temp);
}
} // namespace se

SE_REGISTER_SERIALIZE_TRAITS(se::String)
SE_REGISTER_SERIALIZE_TRAITS(se::StringName)
SE_REGISTER_SERIALIZE_TRAITS(se::Guid)
SE_REGISTER_SERIALIZE_TRAITS(se::TypeId)
SE_REGISTER_SERIALIZE_TRAITS(se::Path)
SE_REGISTER_SERIALIZE_TRAITS(se::VPath)
SE_REGISTER_SERIALIZE_TRAITS(se::ContentHash)
