#pragma once

#include "SimpleEngine/Core/Container/Optional.h"
#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/Reflection/TypeId.h"
#include "SimpleEngine/Core/Reflection/TypeRegistry.h"
#include "SimpleEngine/Core/Serialization/SerializeTraits.h"
#include "SimpleEngine/Core/Types/Guid.h"
#include "SimpleEngine/Core/Types/HashDigest.h"
#include "SimpleEngine/Core/Types/Path.h"
#include "SimpleEngine/Core/Types/StringName.h"
#include "SimpleEngine/Core/Types/VPath.h"


namespace se
{
/** Str 노드로 직접 저장합니다. */
template <>
struct SE_CORE_API SerializeTraits<String>
{
    static constexpr u32 FORMAT_VERSION = 1;

    static void Write(ArchiveWriter& writer, const String& value);
    static void Read(ArchiveReader& reader, String& value);
};

/** 문자열로 저장합니다. */
template <>
struct SE_CORE_API SerializeTraits<StringName>
{
    static constexpr u32 FORMAT_VERSION = 1;

    static void Write(ArchiveWriter& writer, const StringName& value);
    static void Read(ArchiveReader& reader, StringName& value);
};

/** 텍스트 포멧에서는 문자열로, 바이너리에서는 원시 바이트로 저장합니다. */
template <>
struct SE_CORE_API SerializeTraits<Guid>
{
    static constexpr u32 FORMAT_VERSION = 1;

    static void Write(ArchiveWriter& writer, const Guid& value);
    static void Read(ArchiveReader& reader, Guid& value);
};

/** 텍스트 포멧에서는 등록된 이름으로, 바이너리에서는 해시값으로 저장합니다. */
template <>
struct SE_CORE_API SerializeTraits<TypeId>
{
    static constexpr u32 FORMAT_VERSION = 1;

    static void Write(ArchiveWriter& writer, const TypeId& value);
    static void Read(ArchiveReader& reader, TypeId& value);
};

/** 텍스트 포멧에서는 hex 문자열로, 바이너리에서는 원시 바이트로 저장합니다. */
template <usize N>
struct SerializeTraits<HashDigest<N>>
{
    static constexpr u32 FORMAT_VERSION = 1;

    static void Write(ArchiveWriter& writer, const HashDigest<N>& value)
    {
        if (writer.IsTextFormat())
        {
            writer.Str(value.ToHex());
            return;
        }
        writer.Bytes(value.Data(), N);
    }

    static void Read(ArchiveReader& reader, HashDigest<N>& value)
    {
        if (reader.IsTextFormat())
        {
            String hex;
            reader.Str(hex);
            if (reader.HasError())
            {
                return;
            }

            const auto parsed = HashDigest<N>::TryFromHex(hex);
            if (!parsed.HasValue())
            {
                reader.SetError(String::Format("SerializeTraits<HashDigest<{}>>: invalid hex string '{}'.", N, hex));
                return;
            }
            value = parsed.Value();
            return;
        }
        reader.Bytes(value.Data(), N);
    }
};

/** 경로 문자열로 저장합니다. */
template <>
struct SE_CORE_API SerializeTraits<Path>
{
    static constexpr u32 FORMAT_VERSION = 1;

    static void Write(ArchiveWriter& writer, const Path& value);
    static void Read(ArchiveReader& reader, Path& value);
};

/** 경로 문자열로 저장합니다. */
template <>
struct SE_CORE_API SerializeTraits<VPath>
{
    static constexpr u32 FORMAT_VERSION = 1;

    static void Write(ArchiveWriter& writer, const VPath& value);
    static void Read(ArchiveReader& reader, VPath& value);
};
} // namespace se
