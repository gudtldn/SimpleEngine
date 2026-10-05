#include "SimpleEngine/Shader/ShaderBundle.h"

#include "SimpleEngine/Core/Reflection/ReflectMacros.h"
#include "SimpleEngine/Core/Serialization/BinaryArchive.h"
#include "SimpleEngine/Core/Serialization/Serializer.h"
#include "SimpleEngine/Utility/Debug.h"


namespace se
{
Optional<const ShaderBlob&> ShaderBundle::FindBlob(EShaderStage stage, EShaderFormat format) const
{
    return blobs.FindBy([stage, format](const ShaderBlob& blob)
    {
        return blob.stage == stage && blob.format == format;
    });
}

Array<u8> ShaderBundle::Serialize() const
{
    const SerializePlan& plan = SerializePlanOf<ShaderBundle>();

    Array<u8> bytes;
    BinaryFileWriter writer(bytes, plan.type, plan.SchemaHash());
    const auto result = serde::Serialize(writer, plan, this);
    writer.Finish();

    // 메모리 버퍼에 쓰므로 등록이 올바르면 실패하지 않습니다. 실패하면 손상된 번들이 DDC에 남지 않도록 멈춥니다.
    SE_ASSERT_RELEASE(result.HasValue(), "ShaderBundle serialization failed: {} (at '{}')", result.Error().message, result.Error().path);
    return bytes;
}

Expected<ShaderBundle, String> ShaderBundle::Deserialize(ArrayView<const u8> bytes)
{
    const SerializePlan& plan = SerializePlanOf<ShaderBundle>();

    // 루트 타입, 스키마 해시, 체크섬이 맞지 않으면 생성 단계에서 오류 상태가 됩니다.
    BinaryFileReader reader(bytes, plan.type, plan.SchemaHash());
    if (reader.HasError())
    {
        return Unexpected{ String::Format("Rejected shader bundle: {}", reader.GetError()) };
    }

    ShaderBundle bundle;
    if (const auto result = serde::Deserialize(reader, plan, &bundle); result.HasError())
    {
        return Unexpected{ String::Format("Corrupted shader bundle: {} (at '{}')", result.Error().message, result.Error().path) };
    }
    return bundle;
}
} // namespace se


SE_REFLECT_BEGIN(se::ShaderBlob)
    SE_FIELD(stage)
    SE_FIELD(format)
    SE_FIELD(entry_point)
    SE_FIELD(code)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se::ShaderBundle)
    SE_FIELD(program)
    SE_FIELD(blobs)
    SE_FIELD(dependencies)
SE_REFLECT_END()
