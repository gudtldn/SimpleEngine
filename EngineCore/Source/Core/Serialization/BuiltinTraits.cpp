#include "SimpleEngine/Core/Serialization/BuiltinTraits.h"


namespace se
{
// String
void SerializeTraits<String>::Write(Archive& ar, const String& value)
{
    u64 length = value.ByteLen();
    ar.BeginSeq(length);
    ar.Bytes(const_cast<char*>(value.Data()), length);
    ar.EndSeq();
}

void SerializeTraits<String>::Read(Archive& ar, String& value)
{
    u64 length = 0;
    ar.BeginSeq(length);
    value.ResizeUninitialized(length);
    ar.Bytes(value.Data(), length);
    ar.EndSeq();
}


// StringName
void SerializeTraits<StringName>::Write(Archive& ar, const StringName& value)
{
    const String as_string = value.ToString();
    SerializeTraits<String>::Write(ar, as_string);
}

void SerializeTraits<StringName>::Read(Archive& ar, StringName& value)
{
    String temp;
    SerializeTraits<String>::Read(ar, temp);
    value = StringName(temp);
}


// Guid
void SerializeTraits<Guid>::Write(Archive& ar, const Guid& value)
{
    ar.Bytes(const_cast<Guid*>(&value), sizeof(Guid));
}

void SerializeTraits<Guid>::Read(Archive& ar, Guid& value)
{
    ar.Bytes(&value, sizeof(Guid));
}


// TypeId
void SerializeTraits<TypeId>::Write(Archive& ar, const TypeId& value)
{
    i64 packed = static_cast<i64>(value.Value());
    ar.Int(packed, EIntWidth::Bits64, false);
}

void SerializeTraits<TypeId>::Read(Archive& ar, TypeId& value)
{
    i64 packed = 0;
    ar.Int(packed, EIntWidth::Bits64, false);
    value = TypeId::FromRaw(static_cast<u64>(packed));
}


// Path
void SerializeTraits<Path>::Write(Archive& ar, const Path& value)
{
    SerializeTraits<String>::Write(ar, value.ToString());
}

void SerializeTraits<Path>::Read(Archive& ar, Path& value)
{
    String temp;
    SerializeTraits<String>::Read(ar, temp);
    value = Path(temp);
}


// VPath
void SerializeTraits<VPath>::Write(Archive& ar, const VPath& value)
{
    SerializeTraits<String>::Write(ar, value.ToString());
}

void SerializeTraits<VPath>::Read(Archive& ar, VPath& value)
{
    String temp;
    SerializeTraits<String>::Read(ar, temp);
    value = VPath(temp);
}
} // namespace se
