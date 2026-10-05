#if SE_HAS_HLSL_COMPILER

#include "SimpleEngine/Utility/Debug.h"

#include "ShaderCook/SlangReflector.h"

#include <algorithm>
#include <cctype>
#include <utility>


namespace se::editor
{
namespace
{
using SlangKind = slang::TypeReflection::Kind;
using SlangScalar = slang::TypeReflection::ScalarType;

/** 스테이지가 사용하는 descriptor set 번호 */
struct StageSets
{
    u32 resources = 0;
    Optional<u32> readwrite_resources;
    u32 uniform_buffers = 0;
};

/** 보간 값과 그 location */
struct VaryingField
{
    ShaderVarying varying;
    u32 location = 0;
};

/**
 * SDL3 GPU 규약의 스테이지별 set (SPIR-V descriptor set = HLSL register space)
 * https://wiki.libsdl.org/SDL3/SDL_CreateGPUShader#remarks
 * https://wiki.libsdl.org/SDL3/SDL_CreateGPUComputePipeline#remarks
 */
[[nodiscard]] StageSets SetsOf(EShaderStage stage)
{
    switch (stage)
    {
    case EShaderStage::Vertex:   return { .resources = 0, .uniform_buffers = 1 };
    case EShaderStage::Fragment: return { .resources = 2, .uniform_buffers = 3 };
    case EShaderStage::Compute:  return { .resources = 0, .readwrite_resources = 1u, .uniform_buffers = 2 };
    }
    SE_UNREACHABLE();
}

[[nodiscard]] const char* NameOf(slang::VariableLayoutReflection* var)
{
    const char* name = var->getName();
    return name ? name : "";
}

/** 리소스의 형태(텍스처, 버퍼)와 접근 권한(읽기, 읽기/쓰기)으로 종류를 정합니다. */
[[nodiscard]] Optional<EShaderResourceKind> ClassifyShapedResource(slang::TypeLayoutReflection* type_layout)
{
    const u32 base_shape = type_layout->getResourceShape() & SLANG_RESOURCE_BASE_SHAPE_MASK;
    const bool read_write = type_layout->getResourceAccess() == SLANG_RESOURCE_ACCESS_READ_WRITE;

    switch (base_shape)
    {
    case SLANG_TEXTURE_1D:
    case SLANG_TEXTURE_2D:
    case SLANG_TEXTURE_3D:
    case SLANG_TEXTURE_CUBE:
        return read_write ? EShaderResourceKind::ReadWriteStorageTexture : EShaderResourceKind::SampledTexture;

    case SLANG_STRUCTURED_BUFFER:
    case SLANG_BYTE_ADDRESS_BUFFER:
        return read_write ? EShaderResourceKind::ReadWriteStorageBuffer : EShaderResourceKind::StorageBuffer;

    default:
        return NullOpt;
    }
}

/** 전역 파라미터의 리소스 종류. 리소스가 아니면 NullOpt입니다. */
[[nodiscard]] Optional<EShaderResourceKind> ClassifyResource(slang::TypeLayoutReflection* type_layout)
{
    switch (type_layout->getKind())
    {
    case SlangKind::ConstantBuffer: return EShaderResourceKind::UniformBuffer;
    case SlangKind::SamplerState:   return EShaderResourceKind::Sampler;
    case SlangKind::Resource:       return ClassifyShapedResource(type_layout);
    default:                        return NullOpt;
    }
}

[[nodiscard]] EShaderValueType VectorValueType(SlangScalar scalar, usize count)
{
    constexpr EShaderValueType FLOATS[] = { EShaderValueType::Float, EShaderValueType::Float2, EShaderValueType::Float3, EShaderValueType::Float4 };
    constexpr EShaderValueType INTS[] = { EShaderValueType::Int, EShaderValueType::Int2, EShaderValueType::Int3, EShaderValueType::Int4 };
    constexpr EShaderValueType UINTS[] = { EShaderValueType::UInt, EShaderValueType::UInt2, EShaderValueType::UInt3, EShaderValueType::UInt4 };

    if (count < 1 || count > 4)
    {
        return EShaderValueType::Unknown;
    }

    switch (scalar)
    {
    case SlangScalar::Float32: return FLOATS[count - 1];
    case SlangScalar::Int32:   return INTS[count - 1];
    case SlangScalar::UInt32:  return UINTS[count - 1];
    default:                   return EShaderValueType::Unknown;
    }
}

[[nodiscard]] EShaderValueType MatrixValueType(SlangScalar scalar, u32 rows, u32 columns)
{
    if (scalar != SlangScalar::Float32 || rows != columns)
    {
        return EShaderValueType::Unknown;
    }

    switch (rows)
    {
    case 3:  return EShaderValueType::Float3x3;
    case 4:  return EShaderValueType::Float4x4;
    default: return EShaderValueType::Unknown;
    }
}

/** 스칼라, 벡터, 정사각 float 행렬만 변환하고, 나머지는 Unknown입니다. */
[[nodiscard]] EShaderValueType ToValueType(slang::TypeLayoutReflection* type_layout)
{
    const SlangScalar scalar = type_layout->getScalarType();
    switch (type_layout->getKind())
    {
    case SlangKind::Scalar: return VectorValueType(scalar, 1);
    case SlangKind::Vector: return VectorValueType(scalar, type_layout->getElementCount());
    case SlangKind::Matrix: return MatrixValueType(scalar, type_layout->getRowCount(), type_layout->getColumnCount());
    default:                return EShaderValueType::Unknown;
    }
}

[[nodiscard]] ShaderUniformMember ReflectUniformMember(slang::VariableLayoutReflection* field)
{
    slang::TypeLayoutReflection* type_layout = field->getTypeLayout();
    const bool is_array = type_layout->getKind() == SlangKind::Array;

    return {
        .name = NameOf(field),
        .offset = static_cast<u32>(field->getOffset()),
        .size = static_cast<u32>(type_layout->getSize()),
        .type = ToValueType(is_array ? type_layout->getElementTypeLayout() : type_layout),
        .array_count = is_array ? static_cast<u32>(type_layout->getElementCount()) : 0,
    };
}

[[nodiscard]] ShaderUniformBuffer ReflectUniformBuffer(slang::VariableLayoutReflection* param)
{
    slang::TypeLayoutReflection* element = param->getTypeLayout()->getElementTypeLayout();

    ShaderUniformBuffer buffer{
        .name = NameOf(param),
        .slot = param->getBindingIndex(),
        .size = static_cast<u32>(element->getSize()),
    };
    for (usize i = 0; i < element->getFieldCount(); ++i)
    {
        buffer.members.Push(ReflectUniformMember(element->getFieldByIndex(static_cast<u32>(i))));
    }
    return buffer;
}

/** uniform_set에 선언된 상수 버퍼를 읽습니다. SPIR-V set과 DXIL space 번호가 같아 두 레이아웃에 모두 쓸 수 있습니다. */
[[nodiscard]] Array<ShaderUniformBuffer> ReflectUniformBuffers(slang::ProgramLayout* layout, u32 uniform_set)
{
    Array<ShaderUniformBuffer> buffers;
    for (usize i = 0; i < layout->getParameterCount(); ++i)
    {
        slang::VariableLayoutReflection* param = layout->getParameterByIndex(static_cast<u32>(i));
        if (param->getTypeLayout()->getKind() == SlangKind::ConstantBuffer && param->getBindingSpace() == uniform_set)
        {
            buffers.Push(ReflectUniformBuffer(param));
        }
    }
    return buffers;
}

[[nodiscard]] Array<ShaderBindingRecord> CollectDeclaredBindings(slang::ProgramLayout* layout)
{
    Array<ShaderBindingRecord> bindings;
    for (usize i = 0; i < layout->getParameterCount(); ++i)
    {
        slang::VariableLayoutReflection* param = layout->getParameterByIndex(static_cast<u32>(i));
        if (const auto kind = ClassifyResource(param->getTypeLayout()))
        {
            bindings.Push({
                .name = NameOf(param),
                .kind = *kind,
                .space = param->getBindingSpace(),
                .binding = param->getBindingIndex(),
            });
        }
    }
    return bindings;
}

/**
 * space에 선언된 kind 리소스에 SDL 슬롯을 매깁니다. binding이 first_binding인 리소스가 슬롯 0입니다.
 * binding이 first_binding보다 작은 리소스는 규약 위반이므로 제외합니다.
 */
[[nodiscard]] Array<ShaderResourceSlot> PlaceResources(
    const Array<ShaderBindingRecord>& bindings,
    u32 space,
    EShaderResourceKind kind,
    u32 first_binding
)
{
    Array<ShaderResourceSlot> slots;
    for (const ShaderBindingRecord& binding : bindings)
    {
        if (binding.space == space && binding.kind == kind && binding.binding >= first_binding)
        {
            slots.Push({ .name = binding.name, .slot = binding.binding - first_binding });
        }
    }
    return slots;
}

/** SDL에 넘길 개수 (가장 큰 슬롯 + 1) */
template <typename T>
[[nodiscard]] u32 SlotRange(const Array<T>& slots)
{
    u32 range = 0;
    for (const T& slot : slots)
    {
        range = std::max(range, slot.slot + 1);
    }
    return range;
}

/** 리소스 set을 SDL 순서(샘플 텍스처, 스토리지 텍스처, 스토리지 버퍼)대로 나눠 슬롯을 매깁니다. */
void PlaceReadOnlyResources(const Array<ShaderBindingRecord>& bindings, u32 space, ShaderStageInterface& stage_interface)
{
    ShaderResourceCounts& counts = stage_interface.counts;

    stage_interface.sampled_textures = PlaceResources(bindings, space, EShaderResourceKind::SampledTexture, 0);
    stage_interface.samplers = PlaceResources(bindings, space, EShaderResourceKind::Sampler, 0);
    // SDL에서 샘플러는 샘플 텍스처와 짝을 이루므로, 둘 중 큰 범위를 샘플러 개수로 씁니다.
    counts.samplers = std::max(SlotRange(stage_interface.sampled_textures), SlotRange(stage_interface.samplers));

    stage_interface.storage_textures = PlaceResources(bindings, space, EShaderResourceKind::StorageTexture, counts.samplers);
    counts.storage_textures = SlotRange(stage_interface.storage_textures);

    const u32 storage_buffer_base = counts.samplers + counts.storage_textures;
    stage_interface.storage_buffers = PlaceResources(bindings, space, EShaderResourceKind::StorageBuffer, storage_buffer_base);
    counts.storage_buffers = SlotRange(stage_interface.storage_buffers);
}

/** 읽기/쓰기 set을 SDL 순서(텍스처, 버퍼)대로 나눠 슬롯을 매깁니다. */
void PlaceReadWriteResources(const Array<ShaderBindingRecord>& bindings, u32 space, ShaderStageInterface& stage_interface)
{
    ShaderResourceCounts& counts = stage_interface.counts;

    stage_interface.readwrite_storage_textures = PlaceResources(bindings, space, EShaderResourceKind::ReadWriteStorageTexture, 0);
    counts.readwrite_storage_textures = SlotRange(stage_interface.readwrite_storage_textures);

    stage_interface.readwrite_storage_buffers =
        PlaceResources(bindings, space, EShaderResourceKind::ReadWriteStorageBuffer, counts.readwrite_storage_textures);
    counts.readwrite_storage_buffers = SlotRange(stage_interface.readwrite_storage_buffers);
}

/** semantic이 SV_로 시작하는 시스템 값인지 여부 */
[[nodiscard]] bool IsSystemValue(const char* semantic)
{
    return std::toupper(static_cast<unsigned char>(semantic[0])) == 'S'
        && std::toupper(static_cast<unsigned char>(semantic[1])) == 'V'
        && semantic[2] == '_';
}

/** var가 구조체면 필드를 재귀로 펼쳐, 시스템 값이 아닌 보간 값을 모읍니다. 재귀 깊이는 소스의 구조체 중첩 깊이와 같습니다. */
void CollectVaryingFields( // NOLINT(*-no-recursion)
    slang::VariableLayoutReflection* var,
    slang::ParameterCategory category,
    usize base_location,
    Array<VaryingField>& out_fields
)
{
    slang::TypeLayoutReflection* type_layout = var->getTypeLayout();
    const usize location = base_location + var->getOffset(category);

    if (type_layout->getKind() == SlangKind::Struct)
    {
        for (usize i = 0; i < type_layout->getFieldCount(); ++i)
        {
            CollectVaryingFields(type_layout->getFieldByIndex(static_cast<u32>(i)), category, location, out_fields);
        }
        return;
    }

    const char* semantic = var->getSemanticName();
    if (!semantic || IsSystemValue(semantic))
    {
        return;
    }

    out_fields.Push({
        .varying = {
            .name = NameOf(var),
            .semantic = String::Format("{}{}", semantic, var->getSemanticIndex()),
            .type = ToValueType(type_layout),
        },
        .location = static_cast<u32>(location),
    });
}

[[nodiscard]] Array<VaryingField> CollectInputFields(slang::EntryPointReflection* entry)
{
    Array<VaryingField> fields;
    for (usize i = 0; i < entry->getParameterCount(); ++i)
    {
        CollectVaryingFields(entry->getParameterByIndex(static_cast<u32>(i)), slang::ParameterCategory::VaryingInput, 0, fields);
    }
    return fields;
}

[[nodiscard]] Array<ShaderVarying> ToVaryings(Array<VaryingField>&& fields)
{
    Array<ShaderVarying> varyings;
    for (VaryingField& field : fields)
    {
        varyings.Push(std::move(field.varying));
    }
    return varyings;
}

[[nodiscard]] ShaderThreadCount ReflectThreadCount(slang::EntryPointReflection* entry)
{
    SlangUInt sizes[3] = {};
    entry->getComputeThreadGroupSize(3, sizes);
    return { .x = static_cast<u32>(sizes[0]), .y = static_cast<u32>(sizes[1]), .z = static_cast<u32>(sizes[2]) };
}
} // namespace

SlangReflector::SlangReflector(slang::ProgramLayout* spirv_layout, slang::ProgramLayout* dxil_layout)
    : spirv_layout(spirv_layout)
    , dxil_layout(dxil_layout)
    , declared_bindings(CollectDeclaredBindings(spirv_layout))
{
}

ShaderStageInterface SlangReflector::ReflectInterface(SlangUInt entry_index, EShaderStage stage) const
{
    const StageSets sets = SetsOf(stage);
    slang::EntryPointReflection* entry = spirv_layout->getEntryPointByIndex(entry_index);

    ShaderStageInterface stage_interface{ .stage = stage };
    PlaceReadOnlyResources(declared_bindings, sets.resources, stage_interface);
    if (sets.readwrite_resources)
    {
        PlaceReadWriteResources(declared_bindings, *sets.readwrite_resources, stage_interface);
    }

    stage_interface.uniform_buffers = ReflectUniformBuffers(spirv_layout, sets.uniform_buffers);
    stage_interface.counts.uniform_buffers = SlotRange(stage_interface.uniform_buffers);

    if (stage == EShaderStage::Vertex)
    {
        for (const VaryingField& field : CollectInputFields(entry))
        {
            stage_interface.vertex_inputs.Push({ .location = field.location, .name = field.varying.name, .type = field.varying.type });
        }
    }
    if (stage == EShaderStage::Compute)
    {
        stage_interface.thread_count = ReflectThreadCount(entry);
    }
    return stage_interface;
}

Array<ShaderUniformBuffer> SlangReflector::ReflectDxilUniformBuffers(EShaderStage stage) const
{
    return ReflectUniformBuffers(dxil_layout, SetsOf(stage).uniform_buffers);
}

Array<ShaderVarying> SlangReflector::ReflectInputs(SlangUInt entry_index) const
{
    return ToVaryings(CollectInputFields(spirv_layout->getEntryPointByIndex(entry_index)));
}

Array<ShaderVarying> SlangReflector::ReflectOutputs(SlangUInt entry_index) const
{
    slang::VariableLayoutReflection* result = spirv_layout->getEntryPointByIndex(entry_index)->getResultVarLayout();
    if (!result)
    {
        return {};
    }

    Array<VaryingField> fields;
    CollectVaryingFields(result, slang::ParameterCategory::VaryingOutput, 0, fields);
    return ToVaryings(std::move(fields));
}
} // namespace se::editor

#endif
