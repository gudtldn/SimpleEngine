#include "SimpleEngine/Shader/ShaderInterface.h"

#include "SimpleEngine/Core/Reflection/ReflectMacros.h"


namespace se
{
Optional<const ShaderStageInterface&> ShaderProgramInterface::FindStage(EShaderStage stage) const
{
    return stages.FindBy([stage](const ShaderStageInterface& stage_interface)
    {
        return stage_interface.stage == stage;
    });
}
} // namespace se


SE_REFLECT_ENUM_BEGIN(se::EShaderStage)
    SE_ENUM_VALUE(Vertex)
    SE_ENUM_VALUE(Fragment)
    SE_ENUM_VALUE(Compute)
SE_REFLECT_ENUM_END()

SE_REFLECT_ENUM_BEGIN(se::EShaderFormat)
    SE_ENUM_VALUE(SPIRV)
    SE_ENUM_VALUE(DXIL)
SE_REFLECT_ENUM_END()

SE_REFLECT_ENUM_BEGIN(se::EShaderValueType)
    SE_ENUM_VALUE(Unknown)
    SE_ENUM_VALUE(Float)
    SE_ENUM_VALUE(Float2)
    SE_ENUM_VALUE(Float3)
    SE_ENUM_VALUE(Float4)
    SE_ENUM_VALUE(Int)
    SE_ENUM_VALUE(Int2)
    SE_ENUM_VALUE(Int3)
    SE_ENUM_VALUE(Int4)
    SE_ENUM_VALUE(UInt)
    SE_ENUM_VALUE(UInt2)
    SE_ENUM_VALUE(UInt3)
    SE_ENUM_VALUE(UInt4)
    SE_ENUM_VALUE(Float3x3)
    SE_ENUM_VALUE(Float4x4)
SE_REFLECT_ENUM_END()

// 필드는 멤버 선언 순서로 등록합니다.
SE_REFLECT_BEGIN(se::ShaderVertexInput)
    SE_FIELD(location)
    SE_FIELD(name)
    SE_FIELD(type)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se::ShaderUniformMember)
    SE_FIELD(name)
    SE_FIELD(offset)
    SE_FIELD(size)
    SE_FIELD(type)
    SE_FIELD(array_count)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se::ShaderUniformBuffer)
    SE_FIELD(name)
    SE_FIELD(slot)
    SE_FIELD(size)
    SE_FIELD(members)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se::ShaderResourceSlot)
    SE_FIELD(name)
    SE_FIELD(slot)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se::ShaderResourceCounts)
    SE_FIELD(samplers)
    SE_FIELD(storage_textures)
    SE_FIELD(storage_buffers)
    SE_FIELD(readwrite_storage_textures)
    SE_FIELD(readwrite_storage_buffers)
    SE_FIELD(uniform_buffers)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se::ShaderThreadCount)
    SE_FIELD(x)
    SE_FIELD(y)
    SE_FIELD(z)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se::ShaderStageInterface)
    SE_FIELD(stage)
    SE_FIELD(vertex_inputs)
    SE_FIELD(sampled_textures)
    SE_FIELD(samplers)
    SE_FIELD(storage_textures)
    SE_FIELD(storage_buffers)
    SE_FIELD(readwrite_storage_textures)
    SE_FIELD(readwrite_storage_buffers)
    SE_FIELD(uniform_buffers)
    SE_FIELD(counts)
    SE_FIELD(thread_count)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se::ShaderProgramInterface)
    SE_FIELD(stages)
SE_REFLECT_END()
