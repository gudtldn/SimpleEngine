#include "SimpleEngine/Shader/ShaderLibrary.h"

#include "SimpleEngine/Core/Logging/Logging.h"
#include "SimpleEngine/Shader/ShaderBundleSource.h"
#include "SimpleEngine/Utility/Common.h"
#include "SimpleEngine/Utility/Debug.h"

#include "SDL3/SDL_error.h"

#include <ranges>


namespace se
{
namespace
{
/** 번들 포맷과 SDL 포맷의 짝. 앞에 있을수록 먼저 고릅니다. */
struct FormatPreference
{
    EShaderFormat format;
    SDL_GPUShaderFormat sdl_format;
};

constexpr FormatPreference FORMAT_PREFERENCES[] = {
    { .format = EShaderFormat::DXIL, .sdl_format = SDL_GPU_SHADERFORMAT_DXIL },
    { .format = EShaderFormat::SPIRV, .sdl_format = SDL_GPU_SHADERFORMAT_SPIRV },
};

[[nodiscard]] SDL_GPUShaderFormat ToSdlShaderFormat(EShaderFormat format)
{
    switch (format)
    {
    case EShaderFormat::SPIRV: return SDL_GPU_SHADERFORMAT_SPIRV;
    case EShaderFormat::DXIL:  return SDL_GPU_SHADERFORMAT_DXIL;
    }
    return SDL_GPU_SHADERFORMAT_INVALID;
}

/** 로그와 셰이더 이름에 쓰는 스테이지 이름 */
[[nodiscard]] const char* StageName(EShaderStage stage)
{
    switch (stage)
    {
    case EShaderStage::Vertex:   return "vertex";
    case EShaderStage::Fragment: return "fragment";
    case EShaderStage::Compute:  return "compute";
    }
    return "unknown";
}

/** name_property에 이름을 담은 속성을 만듭니다. base가 있으면 그 속성을 복사한 뒤 이름을 더합니다. */
[[nodiscard]] SDL_PropertiesID MakeNamedProperties(const char* name_property, const String& name, SDL_PropertiesID base)
{
    const SDL_PropertiesID props = SDL_CreateProperties();
    if (base != 0)
    {
        SDL_CopyProperties(base, props);
    }
    SDL_SetStringProperty(props, name_property, name.CStr());
    return props;
}
} // namespace


ShaderLibrary::ShaderLibrary(SDL_GPUDevice* in_device, const IShaderBundleSource& in_source)
    : device(in_device)
    , source(in_source)
{
}

ShaderLibrary::~ShaderLibrary()
{
    ClearAll();
}

SDL_GPUShader* ShaderLibrary::GetOrCreateShader(const VPath& program, EShaderStage stage)
{
    SE_ASSERT(stage != EShaderStage::Compute, "Use CreateComputePipeline for compute programs.");

    const auto loaded = FindOrLoad(program);
    if (!loaded)
    {
        return nullptr;
    }

    SDL_GPUShader*& shader = stage == EShaderStage::Vertex ? loaded->vertex_shader : loaded->fragment_shader;
    if (!shader)
    {
        shader = CreateShader(program, loaded->bundle, stage);
    }
    return shader;
}

SDL_GPUComputePipeline* ShaderLibrary::CreateComputePipeline(const VPath& program, SDL_PropertiesID props)
{
    const auto loaded = FindOrLoad(program);
    if (!loaded)
    {
        return nullptr;
    }

    const ShaderBundle& bundle = loaded->bundle;
    const auto stage_interface = bundle.program.FindStage(EShaderStage::Compute);
    const auto blob = ChooseBlob(bundle, EShaderStage::Compute, SDL_GetGPUShaderFormats(device));
    if (!stage_interface || !blob)
    {
        ConsoleLog(ELogLevel::Error, "Shader bundle has no compute stage for this device: {}", program);
        return nullptr;
    }

    const String name = String::Format("{} [compute]", program.ToString());
    const SDL_PropertiesID named_props = MakeNamedProperties(SDL_PROP_GPU_COMPUTEPIPELINE_CREATE_NAME_STRING, name, props);
    SE_SCOPE_DEFER{ SDL_DestroyProperties(named_props); };

    SDL_GPUComputePipelineCreateInfo create_info = MakeComputePipelineCreateInfo(*stage_interface, *blob);
    create_info.props = named_props;

    SDL_GPUComputePipeline* pipeline = SDL_CreateGPUComputePipeline(device, &create_info);
    if (!pipeline)
    {
        ConsoleLog(ELogLevel::Error, "Failed to create compute pipeline: {}, Err: {}", name, SDL_GetError());
    }
    return pipeline;
}

Optional<const ShaderProgramInterface&> ShaderLibrary::FindInterface(const VPath& program) const
{
    const auto loaded = programs.Find(program);
    if (!loaded)
    {
        return NullOpt;
    }
    return loaded->bundle.program;
}

void ShaderLibrary::Invalidate(const VPath& program)
{
    if (const auto loaded = programs.Find(program))
    {
        ReleaseShaders(*loaded);
        programs.Remove(program);
    }
}

void ShaderLibrary::ClearAll()
{
    for (const LoadedProgram& loaded : programs | std::views::values)
    {
        ReleaseShaders(loaded);
    }
    programs.Clear();
}

Optional<const ShaderBlob&> ShaderLibrary::ChooseBlob(
    const ShaderBundle& bundle,
    EShaderStage stage,
    SDL_GPUShaderFormat device_formats
)
{
    for (const FormatPreference& preference : FORMAT_PREFERENCES)
    {
        if ((device_formats & preference.sdl_format) == 0)
        {
            continue;
        }
        if (const auto blob = bundle.FindBlob(stage, preference.format))
        {
            return blob;
        }
    }
    return NullOpt;
}

SDL_GPUShaderCreateInfo ShaderLibrary::MakeShaderCreateInfo(
    const ShaderStageInterface& stage_interface,
    const ShaderBlob& blob
)
{
    SE_ASSERT(stage_interface.stage == blob.stage, "Shader stage interface and blob stage do not match.");
    SE_ASSERT(blob.stage != EShaderStage::Compute, "Use MakeComputePipelineCreateInfo for compute shaders.");

    const ShaderResourceCounts& counts = stage_interface.counts;
    return {
        .code_size = blob.code.Len(),
        .code = blob.code.Data(),
        .entrypoint = blob.entry_point.CStr(),
        .format = ToSdlShaderFormat(blob.format),
        .stage = blob.stage == EShaderStage::Vertex ? SDL_GPU_SHADERSTAGE_VERTEX : SDL_GPU_SHADERSTAGE_FRAGMENT,
        .num_samplers = counts.samplers,
        .num_storage_textures = counts.storage_textures,
        .num_storage_buffers = counts.storage_buffers,
        .num_uniform_buffers = counts.uniform_buffers,
        .props = 0,
    };
}

SDL_GPUComputePipelineCreateInfo ShaderLibrary::MakeComputePipelineCreateInfo(
    const ShaderStageInterface& stage_interface,
    const ShaderBlob& blob
)
{
    SE_ASSERT(stage_interface.stage == EShaderStage::Compute && blob.stage == EShaderStage::Compute, "Compute pipeline needs a compute stage.");

    const ShaderResourceCounts& counts = stage_interface.counts;
    return {
        .code_size = blob.code.Len(),
        .code = blob.code.Data(),
        .entrypoint = blob.entry_point.CStr(),
        .format = ToSdlShaderFormat(blob.format),
        .num_samplers = counts.samplers,
        .num_readonly_storage_textures = counts.storage_textures,
        .num_readonly_storage_buffers = counts.storage_buffers,
        .num_readwrite_storage_textures = counts.readwrite_storage_textures,
        .num_readwrite_storage_buffers = counts.readwrite_storage_buffers,
        .num_uniform_buffers = counts.uniform_buffers,
        .threadcount_x = stage_interface.thread_count.x,
        .threadcount_y = stage_interface.thread_count.y,
        .threadcount_z = stage_interface.thread_count.z,
        .props = 0,
    };
}

Optional<ShaderLibrary::LoadedProgram&> ShaderLibrary::FindOrLoad(const VPath& program)
{
    if (const auto loaded = programs.Find(program))
    {
        return loaded;
    }

    auto bundle = source.Load(program);
    if (!bundle)
    {
        ConsoleLog(ELogLevel::Error, "Failed to load shader program: {}", bundle.Error());
        return NullOpt;
    }
    return programs.Insert(program, LoadedProgram{ .bundle = std::move(*bundle) });
}

void ShaderLibrary::ReleaseShaders(const LoadedProgram& loaded) const
{
    for (SDL_GPUShader* shader : { loaded.vertex_shader, loaded.fragment_shader })
    {
        if (shader)
        {
            SDL_ReleaseGPUShader(device, shader);
        }
    }
}

SDL_GPUShader* ShaderLibrary::CreateShader(const VPath& program, const ShaderBundle& bundle, EShaderStage stage) const
{
    const auto stage_interface = bundle.program.FindStage(stage);
    const auto blob = ChooseBlob(bundle, stage, SDL_GetGPUShaderFormats(device));
    if (!stage_interface || !blob)
    {
        ConsoleLog(ELogLevel::Error, "Shader bundle has no {} stage for this device: {}", StageName(stage), program);
        return nullptr;
    }

    const String name = String::Format("{} [{}]", program.ToString(), StageName(stage));
    const SDL_PropertiesID props = MakeNamedProperties(SDL_PROP_GPU_SHADER_CREATE_NAME_STRING, name, 0);
    SE_SCOPE_DEFER{ SDL_DestroyProperties(props); };

    SDL_GPUShaderCreateInfo create_info = MakeShaderCreateInfo(*stage_interface, *blob);
    create_info.props = props;

    SDL_GPUShader* shader = SDL_CreateGPUShader(device, &create_info);
    if (!shader)
    {
        ConsoleLog(ELogLevel::Error, "Failed to create shader: {}, Err: {}", name, SDL_GetError());
    }
    return shader;
}
} // namespace se
