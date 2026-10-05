// SlangSpike: HLSL 셰이더를 Slang API로 SPIR-V와 DXIL로 컴파일하고 리플렉션을 덤프합니다.
//
// 사용법:
//   SlangSpike --include <dir> --out <dir> [--matrix row|column] [--dx-layout] [--dxc-dir <dir>]
//              <file.hlsl>:<entry>:<vertex|fragment> ...
//
// 셰이더마다 <out>/<name>.<vert|frag>.spv, .dxil, .json을 씁니다.
// JSON에는 SDL_CreateGPUShader에 넘길 리소스 개수와 정점 입력 location, cbuffer 레이아웃이 들어갑니다.

#include <slang.h>
#include <slang-com-ptr.h>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <format>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace fs = std::filesystem;

namespace
{
// 세션의 타깃 순서입니다.
constexpr SlangInt TARGET_SPIRV = 0;
constexpr SlangInt TARGET_DXIL = 1;

struct Job
{
    fs::path file;
    std::string entry;
    SlangStage stage = SLANG_STAGE_NONE;
};

struct Options
{
    std::vector<fs::path> include_dirs;
    fs::path out_dir;
    SlangMatrixLayoutMode matrix_layout = SLANG_MATRIX_LAYOUT_COLUMN_MAJOR;
    bool dx_layout = false;
    std::optional<std::string> dxc_dir;
    std::vector<Job> jobs;
};

void PrintDiagnostics(slang::IBlob* diagnostics)
{
    if (diagnostics && diagnostics->getBufferSize() > 0)
    {
        std::fprintf(stderr, "%s\n", static_cast<const char*>(diagnostics->getBufferPointer()));
    }
}

std::optional<SlangStage> ParseStage(std::string_view text)
{
    if (text == "vertex") { return SLANG_STAGE_VERTEX; }
    if (text == "fragment") { return SLANG_STAGE_FRAGMENT; }
    return std::nullopt;
}

std::optional<Options> ParseArgs(int argc, char** argv)
{
    Options options;
#ifdef SLANG_SPIKE_DXC_DIR
    options.dxc_dir = SLANG_SPIKE_DXC_DIR;
#endif

    for (int i = 1; i < argc; ++i)
    {
        const std::string_view arg = argv[i];
        const bool has_value = i + 1 < argc;

        if (arg == "--include" && has_value) { options.include_dirs.emplace_back(argv[++i]); }
        else if (arg == "--out" && has_value) { options.out_dir = argv[++i]; }
        else if (arg == "--dxc-dir" && has_value) { options.dxc_dir = argv[++i]; }
        else if (arg == "--no-dxc-dir") { options.dxc_dir.reset(); }
        else if (arg == "--dx-layout") { options.dx_layout = true; }
        else if (arg == "--matrix" && has_value)
        {
            const std::string_view value = argv[++i];
            options.matrix_layout = value == "row" ? SLANG_MATRIX_LAYOUT_ROW_MAJOR : SLANG_MATRIX_LAYOUT_COLUMN_MAJOR;
        }
        else
        {
            // <file>:<entry>:<stage>, 파일 경로에 드라이브 문자(C:)가 있을 수 있으므로 뒤에서부터 자릅니다.
            const std::string text(arg);
            const size_t stage_sep = text.rfind(':');
            const size_t entry_sep = stage_sep == std::string::npos ? std::string::npos : text.rfind(':', stage_sep - 1);
            if (entry_sep == std::string::npos)
            {
                std::fprintf(stderr, "Invalid job: %s (expected <file>:<entry>:<stage>)\n", text.c_str());
                return std::nullopt;
            }

            const auto stage = ParseStage(text.substr(stage_sep + 1));
            if (!stage)
            {
                std::fprintf(stderr, "Unknown stage in job: %s\n", text.c_str());
                return std::nullopt;
            }

            options.jobs.push_back({
                .file = text.substr(0, entry_sep),
                .entry = text.substr(entry_sep + 1, stage_sep - entry_sep - 1),
                .stage = *stage,
            });
        }
    }

    if (options.out_dir.empty() || options.jobs.empty())
    {
        std::fprintf(stderr, "Usage: SlangSpike --include <dir> --out <dir> [--matrix row|column] [--dx-layout] "
                             "[--dxc-dir <dir> | --no-dxc-dir] <file.hlsl>:<entry>:<vertex|fragment> ...\n");
        return std::nullopt;
    }
    return options;
}

std::optional<std::string> ReadText(const fs::path& path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file)
    {
        return std::nullopt;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

bool WriteBlob(const fs::path& path, slang::IBlob* blob)
{
    std::ofstream file(path, std::ios::binary);
    file.write(static_cast<const char*>(blob->getBufferPointer()), static_cast<std::streamsize>(blob->getBufferSize()));
    return static_cast<bool>(file);
}

// "Default.vert.hlsl" -> "Default.vert", "Gizmo.hlsl" + vertex -> "Gizmo.vert"
std::string OutputStem(const Job& job)
{
    const std::string stem = job.file.stem().string();
    const std::string suffix = job.stage == SLANG_STAGE_VERTEX ? ".vert" : ".frag";
    return stem.ends_with(suffix) ? stem : stem + suffix;
}

// SDL GPU의 스테이지별 SPIR-V descriptor set입니다.
struct StageSets
{
    unsigned resource_set; // 텍스처, 샘플러, storage 리소스
    unsigned uniform_set;  // 상수 버퍼
};

StageSets SetsForStage(SlangStage stage)
{
    return stage == SLANG_STAGE_VERTEX ? StageSets{ 0, 1 } : StageSets{ 2, 3 };
}

// SDL_CreateGPUShader에 넘길 개수입니다. 개수는 "0부터 N-1까지의 슬롯 범위"이므로 최대 binding + 1로 셉니다.
struct SdlCounts
{
    unsigned samplers = 0;
    unsigned storage_textures = 0;
    unsigned storage_buffers = 0;
    unsigned uniform_buffers = 0;
};

struct ParamInfo
{
    std::string name;
    std::string kind;
    unsigned index = 0;
    unsigned space = 0;
};

std::string KindName(slang::TypeLayoutReflection* type_layout)
{
    switch (type_layout->getKind())
    {
    case slang::TypeReflection::Kind::ConstantBuffer: return "constant_buffer";
    case slang::TypeReflection::Kind::SamplerState: return "sampler";
    case slang::TypeReflection::Kind::Resource:
    {
        const unsigned shape = type_layout->getResourceShape() & SLANG_RESOURCE_BASE_SHAPE_MASK;
        const bool read_write = type_layout->getResourceAccess() == SLANG_RESOURCE_ACCESS_READ_WRITE;
        const bool buffer = shape == SLANG_STRUCTURED_BUFFER || shape == SLANG_BYTE_ADDRESS_BUFFER;
        if (buffer) { return read_write ? "storage_buffer_rw" : "storage_buffer"; }
        if (read_write) { return "storage_texture"; }
        // 결합형 Sampler2D는 텍스처와 샘플러를 한 binding에 담습니다.
        return type_layout->getType()->getKind() == slang::TypeReflection::Kind::Resource
                   && (type_layout->getResourceShape() & SLANG_TEXTURE_COMBINED_FLAG)
                   ? "combined_texture_sampler"
                   : "texture";
    }
    default: return "other";
    }
}

std::vector<ParamInfo> CollectParams(slang::ProgramLayout* layout)
{
    std::vector<ParamInfo> params;
    for (unsigned i = 0; i < layout->getParameterCount(); ++i)
    {
        slang::VariableLayoutReflection* param = layout->getParameterByIndex(i);
        params.push_back({
            .name = param->getName() ? param->getName() : "",
            .kind = KindName(param->getTypeLayout()),
            .index = param->getBindingIndex(),
            .space = param->getBindingSpace(),
        });
    }
    return params;
}

SdlCounts ComputeSdlCounts(const std::vector<ParamInfo>& spirv_params, SlangStage stage)
{
    const StageSets sets = SetsForStage(stage);
    SdlCounts counts;
    for (const ParamInfo& param : spirv_params)
    {
        // 다른 스테이지의 set에 속한 리소스는 이 스테이지가 쓰지 않으므로 셈에서 뺍니다.
        const bool in_resource_set = param.space == sets.resource_set;
        const bool in_uniform_set = param.space == sets.uniform_set;
        const unsigned range = param.index + 1;

        if (in_uniform_set && param.kind == "constant_buffer")
        {
            counts.uniform_buffers = std::max(counts.uniform_buffers, range);
        }
        else if (in_resource_set
                 && (param.kind == "texture" || param.kind == "sampler" || param.kind == "combined_texture_sampler"))
        {
            counts.samplers = std::max(counts.samplers, range);
        }
        else if (in_resource_set && param.kind == "storage_texture")
        {
            counts.storage_textures = std::max(counts.storage_textures, range);
        }
        else if (in_resource_set && param.kind.starts_with("storage_buffer"))
        {
            counts.storage_buffers = std::max(counts.storage_buffers, range);
        }
    }
    return counts;
}

std::string JsonParams(const std::vector<ParamInfo>& params)
{
    std::string out = "[";
    for (size_t i = 0; i < params.size(); ++i)
    {
        const ParamInfo& p = params[i];
        out += std::format("{}{{ \"name\": \"{}\", \"kind\": \"{}\", \"index\": {}, \"space\": {} }}",
                           i == 0 ? "" : ", ", p.name, p.kind, p.index, p.space);
    }
    return out + "]";
}

// cbuffer별 멤버 오프셋과 크기입니다. SPIR-V와 DXIL을 비교하기 위해 타깃마다 따로 씁니다.
std::string JsonCbuffers(slang::ProgramLayout* layout)
{
    std::string out = "[";
    bool first = true;
    for (unsigned i = 0; i < layout->getParameterCount(); ++i)
    {
        slang::VariableLayoutReflection* param = layout->getParameterByIndex(i);
        slang::TypeLayoutReflection* type_layout = param->getTypeLayout();
        if (type_layout->getKind() != slang::TypeReflection::Kind::ConstantBuffer)
        {
            continue;
        }

        slang::TypeLayoutReflection* element = type_layout->getElementTypeLayout();
        std::string members;
        for (unsigned f = 0; f < element->getFieldCount(); ++f)
        {
            slang::VariableLayoutReflection* field = element->getFieldByIndex(f);
            slang::TypeLayoutReflection* field_type = field->getTypeLayout();
            const bool row_major = field_type->getKind() == slang::TypeReflection::Kind::Matrix
                                   && field_type->getMatrixLayoutMode() == SLANG_MATRIX_LAYOUT_ROW_MAJOR;
            members += std::format("{}{{ \"name\": \"{}\", \"offset\": {}, \"size\": {}{} }}",
                                   f == 0 ? "" : ", ", field->getName(), field->getOffset(), field_type->getSize(),
                                   field_type->getKind() == slang::TypeReflection::Kind::Matrix
                                       ? std::format(", \"row_major\": {}", row_major)
                                       : "");
        }

        out += std::format("{}{{ \"name\": \"{}\", \"size\": {}, \"members\": [{}] }}",
                           first ? "" : ", ", param->getName(), element->getSize(), members);
        first = false;
    }
    return out + "]";
}

// 정점 셰이더 입력의 location 목록입니다. SV_* 같은 시스템 값은 varying 입력이 아니므로 빠집니다.
std::vector<size_t> CollectVertexInputs(slang::ProgramLayout* layout)
{
    std::vector<size_t> locations;
    slang::EntryPointReflection* entry = layout->getEntryPointByIndex(0);
    for (unsigned i = 0; i < entry->getParameterCount(); ++i)
    {
        slang::VariableLayoutReflection* param = entry->getParameterByIndex(i);
        slang::TypeLayoutReflection* type_layout = param->getTypeLayout();

        auto push_if_varying = [&](slang::VariableLayoutReflection* var) {
            if (var->getTypeLayout()->getSize(slang::ParameterCategory::VaryingInput) > 0)
            {
                locations.push_back(var->getOffset(slang::ParameterCategory::VaryingInput));
            }
        };

        if (type_layout->getKind() == slang::TypeReflection::Kind::Struct)
        {
            for (unsigned f = 0; f < type_layout->getFieldCount(); ++f)
            {
                push_if_varying(type_layout->getFieldByIndex(f));
            }
        }
        else
        {
            push_if_varying(param);
        }
    }
    return locations;
}

bool CompileJob(slang::ISession* session, const Options& options, const Job& job)
{
    const auto source = ReadText(job.file);
    if (!source)
    {
        std::fprintf(stderr, "Failed to read shader: %s\n", job.file.string().c_str());
        return false;
    }

    const std::string stem = OutputStem(job);
    std::string module_name = stem;
    std::ranges::replace(module_name, '.', '_');

    Slang::ComPtr<slang::IBlob> diagnostics;
    slang::IModule* module =
        session->loadModuleFromSourceString(module_name.c_str(), job.file.string().c_str(), source->c_str(), diagnostics.writeRef());
    PrintDiagnostics(diagnostics);
    if (!module)
    {
        return false;
    }

    Slang::ComPtr<slang::IEntryPoint> entry_point;
    module->findAndCheckEntryPoint(job.entry.c_str(), job.stage, entry_point.writeRef(), diagnostics.writeRef());
    PrintDiagnostics(diagnostics);
    if (!entry_point)
    {
        return false;
    }

    slang::IComponentType* components[] = { module, entry_point };
    Slang::ComPtr<slang::IComponentType> composite;
    session->createCompositeComponentType(components, 2, composite.writeRef(), diagnostics.writeRef());
    PrintDiagnostics(diagnostics);

    Slang::ComPtr<slang::IComponentType> linked;
    composite->link(linked.writeRef(), diagnostics.writeRef());
    PrintDiagnostics(diagnostics);
    if (!linked)
    {
        return false;
    }

    // 타깃별 코드
    for (const auto& [target, extension] : { std::pair{ TARGET_SPIRV, ".spv" }, std::pair{ TARGET_DXIL, ".dxil" } })
    {
        Slang::ComPtr<slang::IBlob> code;
        linked->getEntryPointCode(0, target, code.writeRef(), diagnostics.writeRef());
        PrintDiagnostics(diagnostics);
        if (!code)
        {
            std::fprintf(stderr, "Code generation failed: %s (%s)\n", stem.c_str(), extension);
            return false;
        }
        WriteBlob(options.out_dir / (stem + extension), code);
    }

    // 리플렉션: SDL 개수는 SPIR-V 레이아웃(set/binding)으로 계산합니다.
    slang::ProgramLayout* spirv_layout = linked->getLayout(TARGET_SPIRV);
    slang::ProgramLayout* dxil_layout = linked->getLayout(TARGET_DXIL);
    const std::vector<ParamInfo> spirv_params = CollectParams(spirv_layout);
    const std::vector<ParamInfo> dxil_params = CollectParams(dxil_layout);
    const SdlCounts counts = ComputeSdlCounts(spirv_params, job.stage);

    std::string vertex_inputs = "[";
    if (job.stage == SLANG_STAGE_VERTEX)
    {
        const std::vector<size_t> locations = CollectVertexInputs(spirv_layout);
        for (size_t i = 0; i < locations.size(); ++i)
        {
            vertex_inputs += std::format("{}{}", i == 0 ? "" : ", ", locations[i]);
        }
    }
    vertex_inputs += "]";

    const char* entry_name_override = spirv_layout->getEntryPointByIndex(0)->getNameOverride();
    const std::string json = std::format(
        "{{\n"
        "  \"entry\": \"{}\",\n"
        "  \"stage\": \"{}\",\n"
        "  \"samplers\": {}, \"storage_textures\": {}, \"storage_buffers\": {}, \"uniform_buffers\": {},\n"
        "  \"vertex_inputs\": {},\n"
        "  \"spirv_params\": {},\n"
        "  \"dxil_params\": {},\n"
        "  \"spirv_cbuffers\": {},\n"
        "  \"dxil_cbuffers\": {}\n"
        "}}\n",
        entry_name_override ? entry_name_override : job.entry,
        job.stage == SLANG_STAGE_VERTEX ? "vertex" : "fragment",
        counts.samplers, counts.storage_textures, counts.storage_buffers, counts.uniform_buffers,
        vertex_inputs,
        JsonParams(spirv_params),
        JsonParams(dxil_params),
        JsonCbuffers(spirv_layout),
        JsonCbuffers(dxil_layout));

    std::ofstream(options.out_dir / (stem + ".json")) << json;
    std::printf("OK %s: samplers=%u storage_textures=%u storage_buffers=%u uniform_buffers=%u\n", stem.c_str(),
                counts.samplers, counts.storage_textures, counts.storage_buffers, counts.uniform_buffers);
    return true;
}
} // namespace

int main(int argc, char** argv)
{
    const auto options = ParseArgs(argc, argv);
    if (!options)
    {
        return 1;
    }
    fs::create_directories(options->out_dir);

    Slang::ComPtr<slang::IGlobalSession> global_session;
    if (SLANG_FAILED(slang::createGlobalSession(global_session.writeRef())))
    {
        std::fprintf(stderr, "Failed to create Slang global session\n");
        return 1;
    }

    // DXIL 생성은 DXC(dxcompiler.dll)에 맡기므로 그 위치를 알려 줍니다.
    if (options->dxc_dir)
    {
        global_session->setDownstreamCompilerPath(SLANG_PASS_THROUGH_DXC, options->dxc_dir->c_str());
    }

    // SDL GPU 요구: SPIR-V 1.0(Vulkan 1.0), D3D12 셰이더 모델 6.0
    const slang::TargetDesc targets[] = {
        { .format = SLANG_SPIRV, .profile = global_session->findProfile("spirv_1_0") },
        { .format = SLANG_DXIL, .profile = global_session->findProfile("sm_6_0") },
    };

    std::vector<slang::CompilerOptionEntry> compiler_options = {
        // SPIR-V 진입점 이름을 소스 이름(VSMain 등) 그대로 둡니다. 기본은 "main"으로 바뀝니다.
        { slang::CompilerOptionName::VulkanUseEntryPointName, { .kind = slang::CompilerOptionValueKind::Int, .intValue0 = 1 } },
    };
    if (options->dx_layout)
    {
        // SPIR-V cbuffer도 D3D 패킹 규칙으로 배치합니다.
        compiler_options.push_back(
            { slang::CompilerOptionName::ForceDXLayout, { .kind = slang::CompilerOptionValueKind::Int, .intValue0 = 1 } });
    }

    std::vector<std::string> include_strings;
    std::vector<const char*> search_paths;
    for (const fs::path& dir : options->include_dirs)
    {
        include_strings.push_back(dir.string());
    }
    for (const std::string& dir : include_strings)
    {
        search_paths.push_back(dir.c_str());
    }

    slang::SessionDesc session_desc = {};
    session_desc.targets = targets;
    session_desc.targetCount = 2;
    session_desc.defaultMatrixLayoutMode = options->matrix_layout;
    session_desc.searchPaths = search_paths.data();
    session_desc.searchPathCount = static_cast<SlangInt>(search_paths.size());
    session_desc.compilerOptionEntries = compiler_options.data();
    session_desc.compilerOptionEntryCount = static_cast<uint32_t>(compiler_options.size());

    Slang::ComPtr<slang::ISession> session;
    if (SLANG_FAILED(global_session->createSession(session_desc, session.writeRef())))
    {
        std::fprintf(stderr, "Failed to create Slang session\n");
        return 1;
    }

    bool all_ok = true;
    for (const Job& job : options->jobs)
    {
        all_ok &= CompileJob(session, *options, job);
    }
    return all_ok ? 0 : 1;
}
