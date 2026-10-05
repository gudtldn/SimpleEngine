#include "SimpleEditor/ShaderCook/ShaderCompiler.h"

#include "SimpleEngine/Utility/Debug.h"

#include "ShaderCook/ShaderStageSets.h"


namespace se::editor
{
namespace
{
// 스테이지당 SDL 리소스 한계 (SDL 소스 src/gpu/SDL_sysgpu.h)
constexpr u32 MAX_SAMPLERS = 16;
constexpr u32 MAX_STORAGE_TEXTURES = 8;
constexpr u32 MAX_STORAGE_BUFFERS = 8;
constexpr u32 MAX_READWRITE_STORAGE_TEXTURES = 8;
constexpr u32 MAX_READWRITE_STORAGE_BUFFERS = 8;
constexpr u32 MAX_UNIFORM_BUFFERS = 4;

/** SDL 한계와 비교할 리소스 개수 */
struct ResourceLimit
{
    const char* name;
    u32 count;
    u32 limit;
};

[[nodiscard]] const char* StageName(EShaderStage stage)
{
    switch (stage)
    {
    case EShaderStage::Vertex:   return "vertex";
    case EShaderStage::Fragment: return "fragment";
    case EShaderStage::Compute:  return "compute";
    }
    SE_UNREACHABLE();
}

[[nodiscard]] const char* KindName(EShaderResourceKind kind)
{
    switch (kind)
    {
    case EShaderResourceKind::UniformBuffer:           return "uniform buffer";
    case EShaderResourceKind::SampledTexture:          return "sampled texture";
    case EShaderResourceKind::Sampler:                 return "sampler";
    case EShaderResourceKind::StorageTexture:          return "storage texture";
    case EShaderResourceKind::StorageBuffer:           return "storage buffer";
    case EShaderResourceKind::ReadWriteStorageTexture: return "read-write storage texture";
    case EShaderResourceKind::ReadWriteStorageBuffer:  return "read-write storage buffer";
    }
    SE_UNREACHABLE();
}

/** kind 리소스를 선언하는 HLSL register 문자 */
[[nodiscard]] char RegisterLetter(EShaderResourceKind kind)
{
    switch (kind)
    {
    case EShaderResourceKind::UniformBuffer:           return 'b';
    case EShaderResourceKind::Sampler:                 return 's';
    case EShaderResourceKind::SampledTexture:
    case EShaderResourceKind::StorageTexture:
    case EShaderResourceKind::StorageBuffer:           return 't';
    case EShaderResourceKind::ReadWriteStorageTexture:
    case EShaderResourceKind::ReadWriteStorageBuffer:  return 'u';
    }
    SE_UNREACHABLE();
}

/** 문제 한 줄에 원인(note)과 고치는 방법(help)을 붙입니다. 비어 있는 항목은 생략합니다. */
[[nodiscard]] String Diagnostic(StringView message, StringView note, StringView help)
{
    String text = message;
    if (!note.IsEmpty())
    {
        text += "\n  note: ";
        text += note;
    }
    if (!help.IsEmpty())
    {
        text += "\n  help: ";
        text += help;
    }
    return text;
}

/** stage에서 kind 리소스를 선언해야 하는 set. 그 스테이지에 둘 수 없는 종류면 NullOpt입니다. */
[[nodiscard]] Optional<u32> ExpectedSpace(EShaderStage stage, EShaderResourceKind kind)
{
    const StageSets sets = SetsOf(stage);
    switch (kind)
    {
    case EShaderResourceKind::UniformBuffer:
        return sets.uniform_buffers;
    case EShaderResourceKind::ReadWriteStorageTexture:
    case EShaderResourceKind::ReadWriteStorageBuffer:
        return sets.readwrite_resources;
    default:
        return sets.resources;
    }
}

/** 컴퓨트 스테이지는 다른 스테이지와 한 파일에 둘 수 없습니다. 같은 space가 스테이지마다 다른 종류를 뜻하기 때문입니다. */
void ValidateStageMix(const CompiledShaderProgram& program, Array<String>& problems)
{
    if (program.FindStage(EShaderStage::Compute).HasValue() && program.stages.Len() > 1)
    {
        problems.Push(Diagnostic(
            "Compute entry point cannot share a file with graphics stages",
            "space1 holds vertex uniform buffers but compute read-write resources, so the declarations are ambiguous",
            "move the compute entry point to its own file"
        ));
    }
}

/**
 * 선언된 리소스마다 그 종류와 space를 받아 주는 스테이지가 있는지 검사합니다.
 * 선언에는 어느 스테이지가 쓰는지 정보가 없으므로, 다른 스테이지의 set에 둔 리소스는 잡지 못합니다.
 */
void ValidateDeclarations(const CompiledShaderProgram& program, Array<String>& problems)
{
    for (const ShaderBindingRecord& binding : program.declared_bindings)
    {
        String expected;
        String suggestions;
        bool accepted = false;
        for (const CompiledShaderStage& stage : program.stages)
        {
            const Optional<u32> space = ExpectedSpace(stage.stage, binding.kind);
            if (!space)
            {
                continue;
            }
            accepted = accepted || *space == binding.space;
            if (!expected.IsEmpty())
            {
                expected += " or ";
                suggestions += ", or ";
            }
            expected += String::Format("space{} ({})", *space, StageName(stage.stage));
            suggestions += String::Format(
                "register({}{}, space{}) for the {} stage", RegisterLetter(binding.kind), binding.binding, *space, StageName(stage.stage)
            );
        }

        if (accepted)
        {
            continue;
        }
        if (expected.IsEmpty())
        {
            problems.Push(Diagnostic(
                String::Format("Resource '{}' ({}) is only allowed in compute shaders", binding.name, KindName(binding.kind)),
                "SDL3 GPU graphics stages have no read-write resource set",
                "use a read-only type such as Texture2D or StructuredBuffer, or move it to a compute shader"
            ));
            continue;
        }
        problems.Push(Diagnostic(
            String::Format("Resource '{}' ({}) is declared in space{}", binding.name, KindName(binding.kind), binding.space),
            String::Format("SDL3 GPU expects {}s in {}", KindName(binding.kind), expected),
            String::Format("use {}", suggestions)
        ));
    }
}

/** binding이 stage의 스토리지 리소스일 때, 앞 종류가 차지한 범위 바로 다음 번호. 그 밖이면 NullOpt입니다. */
[[nodiscard]] Optional<u32> FirstStorageBinding(const ShaderBindingRecord& binding, const ShaderStageInterface& stage_interface)
{
    const StageSets sets = SetsOf(stage_interface.stage);
    const ShaderResourceCounts& counts = stage_interface.counts;

    if (binding.space == sets.resources)
    {
        if (binding.kind == EShaderResourceKind::StorageTexture)
        {
            return counts.samplers;
        }
        if (binding.kind == EShaderResourceKind::StorageBuffer)
        {
            return counts.samplers + counts.storage_textures;
        }
    }
    if (sets.readwrite_resources && binding.space == *sets.readwrite_resources && binding.kind == EShaderResourceKind::ReadWriteStorageBuffer)
    {
        return counts.readwrite_storage_textures;
    }
    return NullOpt;
}

/** 스토리지 리소스가 같은 set의 앞 종류 범위 안쪽에 선언되지 않았는지 검사합니다. */
void ValidateStorageRanges(const CompiledShaderProgram& program, const CompiledShaderStage& stage, Array<String>& problems)
{
    for (const ShaderBindingRecord& binding : program.declared_bindings)
    {
        const Optional<u32> first = FirstStorageBinding(binding, stage.stage_interface);
        if (!first || binding.binding >= *first)
        {
            continue;
        }

        const bool is_readwrite = binding.kind == EShaderResourceKind::ReadWriteStorageBuffer;
        problems.Push(Diagnostic(
            String::Format(
                "[{}] {} '{}' uses binding {}, inside the earlier resource range 0..{}",
                StageName(stage.stage), KindName(binding.kind), binding.name, binding.binding, *first - 1
            ),
            is_readwrite
                ? "SDL3 GPU numbers read-write storage textures first, then read-write storage buffers"
                : "SDL3 GPU numbers sampled textures and samplers first, then storage textures, then storage buffers",
            String::Format("use register({}{}, space{}) or a higher number", RegisterLetter(binding.kind), *first, binding.space)
        ));
    }
}

/** 리소스 개수가 SDL 한계 이하인지 검사합니다. */
void ValidateLimits(const CompiledShaderStage& stage, Array<String>& problems)
{
    const ShaderResourceCounts& counts = stage.stage_interface.counts;
    const ResourceLimit limits[] = {
        { .name = "sampler",                    .count = counts.samplers,                   .limit = MAX_SAMPLERS                   },
        { .name = "storage texture",            .count = counts.storage_textures,           .limit = MAX_STORAGE_TEXTURES           },
        { .name = "storage buffer",             .count = counts.storage_buffers,            .limit = MAX_STORAGE_BUFFERS            },
        { .name = "read-write storage texture", .count = counts.readwrite_storage_textures, .limit = MAX_READWRITE_STORAGE_TEXTURES },
        { .name = "read-write storage buffer",  .count = counts.readwrite_storage_buffers,  .limit = MAX_READWRITE_STORAGE_BUFFERS  },
        { .name = "uniform buffer",             .count = counts.uniform_buffers,            .limit = MAX_UNIFORM_BUFFERS            },
    };
    for (const auto& [name, count, limit] : limits)
    {
        if (count > limit)
        {
            problems.Push(Diagnostic(
                String::Format("[{}] {} count {} exceeds limit {}", StageName(stage.stage), name, count, limit),
                "the count is the highest slot + 1, so gaps between registers also count",
                "remove unused declarations or close the gaps between register numbers"
            ));
        }
    }
}

/** 상수 버퍼 멤버의 오프셋이 SPIR-V와 DXIL에서 같은지 검사합니다. */
void ValidateUniformMembers(const char* stage_name, const ShaderUniformBuffer& spirv, const ShaderUniformBuffer& dxil, Array<String>& problems)
{
    for (const ShaderUniformMember& member : spirv.members)
    {
        const auto dxil_member = dxil.members.FindBy([&member](const ShaderUniformMember& other) { return other.name == member.name; });
        if (!dxil_member)
        {
            problems.Push(String::Format(
                "[{}] uniform buffer '{}' member '{}' is missing from the DXIL layout", stage_name, spirv.name, member.name
            ));
            continue;
        }
        if (dxil_member->offset != member.offset)
        {
            problems.Push(Diagnostic(
                String::Format(
                    "[{}] uniform buffer '{}' member '{}' offset differs: SPIR-V {}, DXIL {}",
                    stage_name, spirv.name, member.name, member.offset, dxil_member->offset
                ),
                "both formats are compiled with D3D constant buffer packing, so their offsets should be identical",
                "align the member to a 16-byte boundary, or add explicit padding before it"
            ));
        }
    }
}

/** 상수 버퍼의 크기와 멤버 오프셋이 SPIR-V와 DXIL에서 같은지 검사합니다. */
void ValidateUniformLayouts(const CompiledShaderStage& stage, Array<String>& problems)
{
    const char* stage_name = StageName(stage.stage);
    for (const ShaderUniformBuffer& spirv : stage.stage_interface.uniform_buffers)
    {
        const auto dxil = stage.dxil_uniform_buffers.FindBy([&spirv](const ShaderUniformBuffer& buffer) { return buffer.name == spirv.name; });
        if (!dxil)
        {
            problems.Push(String::Format("[{}] uniform buffer '{}' is missing from the DXIL layout", stage_name, spirv.name));
            continue;
        }
        if (spirv.size != dxil->size)
        {
            problems.Push(String::Format("[{}] uniform buffer '{}' size differs: SPIR-V {}, DXIL {}", stage_name, spirv.name, spirv.size, dxil->size));
        }
        ValidateUniformMembers(stage_name, spirv, *dxil, problems);
    }
}

/**
 * 픽셀 입력마다 같은 semantic, location, 타입의 정점 출력이 있는지 검사합니다.
 * D3D는 semantic으로, Vulkan은 location으로 두 스테이지를 연결하므로 둘 다 맞아야 합니다.
 */
void ValidateVaryings(const CompiledShaderProgram& program, Array<String>& problems)
{
    const auto vertex = program.FindStage(EShaderStage::Vertex);
    const auto fragment = program.FindStage(EShaderStage::Fragment);
    if (!vertex || !fragment)
    {
        return;
    }

    for (const ShaderVarying& input : fragment->inputs)
    {
        const auto output = vertex->outputs.FindBy([&input](const ShaderVarying& varying) { return varying.semantic == input.semantic; });
        if (!output)
        {
            problems.Push(Diagnostic(
                String::Format("[fragment] input '{}' ({}) is not written by the vertex stage", input.name, input.semantic),
                "",
                String::Format("add a {} output to the vertex stage, or remove this input", input.semantic)
            ));
            continue;
        }
        if (output->location != input.location)
        {
            problems.Push(Diagnostic(
                String::Format(
                    "[fragment] input '{}' ({}) is at location {}, but the vertex stage writes it at location {}",
                    input.name, input.semantic, input.location, output->location
                ),
                "Vulkan links stage varyings by location and D3D by semantic, so both must match",
                "declare the fragment inputs in the same order as the vertex outputs, or share one struct"
            ));
        }
        if (output->type != input.type)
        {
            problems.Push(Diagnostic(
                String::Format("[fragment] input '{}' ({}) type differs from vertex output '{}'", input.name, input.semantic, output->name),
                "",
                String::Format("declare '{}' with the same type as '{}'", input.name, output->name)
            ));
        }
    }
}

/** 정점 입력의 semantic이 TEXCOORD{location}인지 검사합니다. SDL의 D3D12 백엔드가 이 이름으로 정점 속성을 연결합니다. */
void ValidateVertexInputs(const CompiledShaderProgram& program, Array<String>& problems)
{
    const auto vertex = program.FindStage(EShaderStage::Vertex);
    if (!vertex)
    {
        return;
    }

    for (const ShaderVarying& input : vertex->inputs)
    {
        const String expected = String::Format("TEXCOORD{}", input.location);
        if (input.semantic != expected)
        {
            problems.Push(Diagnostic(
                String::Format("[vertex] input '{}' uses {}", input.name, input.semantic),
                "the SDL3 GPU D3D12 backend binds vertex attributes by TEXCOORD{location}",
                String::Format("use ': {}' to match location {}", expected, input.location)
            ));
        }
    }
}
} // namespace

Array<String> CompiledShaderProgram::Validate() const
{
    Array<String> problems;
    ValidateStageMix(*this, problems);
    ValidateDeclarations(*this, problems);
    for (const CompiledShaderStage& stage : stages)
    {
        ValidateStorageRanges(*this, stage, problems);
        ValidateLimits(stage, problems);
        ValidateUniformLayouts(stage, problems);
    }
    ValidateVaryings(*this, problems);
    ValidateVertexInputs(*this, problems);
    return problems;
}
} // namespace se::editor
