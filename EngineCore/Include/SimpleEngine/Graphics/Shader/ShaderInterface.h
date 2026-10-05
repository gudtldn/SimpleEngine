#pragma once

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/Optional.h"
#include "SimpleEngine/Core/Reflection/Registrar.h"
#include "SimpleEngine/Core/Types/StringName.h"


namespace se
{
/** 셰이더 스테이지 */
enum class EShaderStage : u8
{
    Vertex,
    Fragment,
};

/** 셰이더 바이트코드 포맷 */
enum class EShaderFormat : u8
{
    SPIRV,
    DXIL,
};

/** 정점 입력과 상수 버퍼 멤버의 값 타입 */
enum class EShaderValueType : u8
{
    Unknown,
    Float,
    Float2,
    Float3,
    Float4,
    Int,
    Int2,
    Int3,
    Int4,
    UInt,
    UInt2,
    UInt3,
    UInt4,
    Float3x3,
    Float4x4,
};

/** 정점 셰이더 입력 하나 */
struct ShaderVertexInput
{
    u32 location = 0;
    StringName name;
    EShaderValueType type = EShaderValueType::Unknown;

    [[nodiscard]] bool operator==(const ShaderVertexInput&) const = default;
};

/** 상수 버퍼 멤버 하나. offset과 size는 바이트 단위입니다. */
struct ShaderUniformMember
{
    StringName name;
    u32 offset = 0;
    u32 size = 0;
    EShaderValueType type = EShaderValueType::Unknown;

    /** 배열이 아니면 0입니다. */
    u32 array_count = 0;

    [[nodiscard]] bool operator==(const ShaderUniformMember&) const = default;
};

/** 상수 버퍼 하나. slot은 SDL 유니폼 슬롯입니다. */
struct ShaderUniformBuffer
{
    StringName name;
    u32 slot = 0;
    u32 size = 0;
    Array<ShaderUniformMember> members;

    [[nodiscard]] bool operator==(const ShaderUniformBuffer&) const = default;
};

/** 텍스처, 샘플러, 스토리지 리소스가 차지하는 SDL 슬롯 */
struct ShaderResourceSlot
{
    StringName name;
    u32 slot = 0;

    [[nodiscard]] bool operator==(const ShaderResourceSlot&) const = default;
};

/** SDL_GPUShaderCreateInfo에 그대로 넘기는 리소스 슬롯 범위 */
struct ShaderResourceCounts
{
    u32 samplers = 0;
    u32 storage_textures = 0;
    u32 storage_buffers = 0;
    u32 uniform_buffers = 0;

    [[nodiscard]] bool operator==(const ShaderResourceCounts&) const = default;
};

/** 스테이지 하나가 C++과 맺는 계약 */
struct ShaderStageInterface
{
    EShaderStage stage = EShaderStage::Vertex;

    /** 정점 스테이지에만 있습니다. */
    Array<ShaderVertexInput> vertex_inputs;

    Array<ShaderResourceSlot> sampled_textures;
    Array<ShaderResourceSlot> samplers;
    Array<ShaderResourceSlot> storage_textures;
    Array<ShaderResourceSlot> storage_buffers;
    Array<ShaderUniformBuffer> uniform_buffers;
    ShaderResourceCounts counts;

    [[nodiscard]] bool operator==(const ShaderStageInterface&) const = default;
};

/** 셰이더 프로그램(함께 쓰는 스테이지 묶음)의 계약 */
struct SE_CORE_API ShaderProgramInterface
{
    Array<ShaderStageInterface> stages;

    /** stage의 계약을 찾습니다. 프로그램에 그 스테이지가 없으면 NullOpt입니다. */
    [[nodiscard]] Optional<const ShaderStageInterface&> FindStage(EShaderStage stage) const;

    [[nodiscard]] bool operator==(const ShaderProgramInterface&) const = default;
};
} // namespace se

SE_DECLARE_REFLECTION(se::EShaderStage, SE_CORE_API)
SE_DECLARE_REFLECTION(se::EShaderFormat, SE_CORE_API)
SE_DECLARE_REFLECTION(se::EShaderValueType, SE_CORE_API)
SE_DECLARE_REFLECTION(se::ShaderVertexInput, SE_CORE_API)
SE_DECLARE_REFLECTION(se::ShaderUniformMember, SE_CORE_API)
SE_DECLARE_REFLECTION(se::ShaderUniformBuffer, SE_CORE_API)
SE_DECLARE_REFLECTION(se::ShaderResourceSlot, SE_CORE_API)
SE_DECLARE_REFLECTION(se::ShaderResourceCounts, SE_CORE_API)
SE_DECLARE_REFLECTION(se::ShaderStageInterface, SE_CORE_API)
SE_DECLARE_REFLECTION(se::ShaderProgramInterface, SE_CORE_API)
