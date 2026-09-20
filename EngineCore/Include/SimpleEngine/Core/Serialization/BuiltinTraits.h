#pragma once

#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/Reflection/TypeId.h"
#include "SimpleEngine/Core/Serialization/SerializeTraits.h"
#include "SimpleEngine/Core/Types/Guid.h"
#include "SimpleEngine/Core/Types/HashDigest.h"
#include "SimpleEngine/Core/Types/Path.h"
#include "SimpleEngine/Core/Types/StringName.h"
#include "SimpleEngine/Core/Types/VPath.h"


namespace se
{
/** 길이 접두(BeginSeq) + 원시 바이트로 저장합니다. */
template <>
struct SE_CORE_API SerializeTraits<String>
{
    static void Write(Archive& ar, const String& value);
    static void Read(Archive& ar, String& value);
};

/** 실제 문자열 내용으로 저장합니다. (String Trait에 위임) */
template <>
struct SE_CORE_API SerializeTraits<StringName>
{
    static void Write(Archive& ar, const StringName& value);
    static void Read(Archive& ar, StringName& value);
};

/** 128비트 고정 크기라 길이 접두 없이 원시 바이트로 저장합니다. */
template <>
struct SE_CORE_API SerializeTraits<Guid>
{
    static void Write(Archive& ar, const Guid& value);
    static void Read(Archive& ar, Guid& value);
};

/** 8바이트 해시값을 그대로 저장합니다. */
template <>
struct SE_CORE_API SerializeTraits<TypeId>
{
    static void Write(Archive& ar, const TypeId& value);
    static void Read(Archive& ar, TypeId& value);
};

/** 고정 크기(N바이트) 해시라 길이 접두 없이 원시 바이트로 저장합니다. */
template <usize N>
struct SerializeTraits<HashDigest<N>>
{
    static void Write(Archive& ar, const HashDigest<N>& value)
    {
        ar.Bytes(const_cast<u8*>(value.Data()), N);
    }

    static void Read(Archive& ar, HashDigest<N>& value)
    {
        ar.Bytes(value.Data(), N);
    }
};

/** 내부 String을 그대로 String Trait에 위임합니다. */
template <>
struct SE_CORE_API SerializeTraits<Path>
{
    static void Write(Archive& ar, const Path& value);
    static void Read(Archive& ar, Path& value);
};

/** 내부 String을 그대로 String Trait에 위임합니다. */
template <>
struct SE_CORE_API SerializeTraits<VPath>
{
    static void Write(Archive& ar, const VPath& value);
    static void Read(Archive& ar, VPath& value);
};
} // namespace se
