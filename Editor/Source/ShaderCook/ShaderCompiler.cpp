#include "SimpleEditor/ShaderCook/ShaderCompiler.h"

#if SE_HAS_HLSL_COMPILER
#include "ShaderCook/SlangReflector.h"

#include "SimpleEngine/Core/FileSystem/FileSystem.h"
#include "SimpleEngine/Core/HAL/Platform.h"

#include <slang-com-ptr.h>
#include <slang.h>

#include <iterator>
#endif


namespace se::editor
{
Optional<const CompiledShaderStage&> CompiledShaderProgram::FindStage(EShaderStage stage) const
{
    return stages.FindBy([stage](const CompiledShaderStage& compiled) { return compiled.stage == stage; });
}

#if SE_HAS_HLSL_COMPILER
namespace
{
/** 세션에 등록한 타깃의 인덱스 */
constexpr SlangInt TARGET_SPIRV = 0;
constexpr SlangInt TARGET_DXIL = 1;

/** 끄는 경고 (모두 무해): vk::binding 미지정, 텍스처와 샘플러의 binding 중복, SPIR-V 1.0 지정 */
constexpr const char* DISABLED_WARNINGS = "39029,39001,50011";

/** 세션 고정 옵션의 설명. 툴체인 식별 문자열에 들어가므로 옵션을 바꾸면 함께 고쳐야 합니다. */
constexpr const char* FIXED_OPTIONS = "spirv_1_0 sm_6_0 column_major ForceDXLayout VulkanUseEntryPointName";

struct TargetFormat
{
    SlangInt target;
    EShaderFormat format;
};

constexpr TargetFormat TARGET_FORMATS[] = {
    { .target = TARGET_SPIRV, .format = EShaderFormat::SPIRV },
    { .target = TARGET_DXIL,  .format = EShaderFormat::DXIL  },
};

/** Slang 모듈 이름. 컴파일마다 새 세션을 쓰므로 이름이 겹치지 않습니다. */
constexpr const char* MODULE_NAME = "ShaderSource";

[[nodiscard]] String ToString(slang::IBlob* blob)
{
    if (!blob || blob->getBufferSize() == 0)
    {
        return {};
    }
    return StringView(static_cast<const char*>(blob->getBufferPointer()), blob->getBufferSize());
}

void AppendDiagnostics(String& diagnostics, slang::IBlob* blob)
{
    diagnostics.Append(ToString(blob));
}

[[nodiscard]] Array<u8> ToBytes(slang::IBlob* blob)
{
    const u8* data = static_cast<const u8*>(blob->getBufferPointer());
    Array<u8> bytes;
    bytes.Push(data, data + blob->getBufferSize());
    return bytes;
}

[[nodiscard]] Optional<EShaderStage> ToShaderStage(SlangStage stage)
{
    switch (stage)
    {
    case SLANG_STAGE_VERTEX:   return EShaderStage::Vertex;
    case SLANG_STAGE_FRAGMENT: return EShaderStage::Fragment;
    case SLANG_STAGE_COMPUTE:  return EShaderStage::Compute;
    default:                   return NullOpt;
    }
}

[[nodiscard]] slang::CompilerOptionEntry EnableOption(slang::CompilerOptionName name)
{
    return { .name = name, .value = { .kind = slang::CompilerOptionValueKind::Int, .intValue0 = 1 } };
}

[[nodiscard]] ShaderCookError CompileError(const Path& source_path, StringView what, const String& diagnostics)
{
    return {
        ShaderCookError::CompileFailed,
        String::Format("{}: {}\n{}", what, source_path, diagnostics),
        source_path,
    };
}

[[nodiscard]] ShaderCookResult<String> ReadSource(const Path& source_path)
{
    auto result = fs::ReadToString(source_path);
    if (result)
    {
        return std::move(result).Value();
    }

    return Unexpected<ShaderCookError>{
        ShaderCookError::ReadFailed,
        String::Format("Failed to read shader source: {}, Err: {}", source_path, result.Error().What()),
        source_path,
    };
}

/** [shader] 속성이 붙은 모든 진입점을 모읍니다. */
[[nodiscard]] ShaderCookResult<Array<Slang::ComPtr<slang::IEntryPoint>>> CollectEntryPoints(slang::IModule* module, const Path& source_path)
{
    Array<Slang::ComPtr<slang::IEntryPoint>> entry_points;
    for (SlangInt32 i = 0; i < module->getDefinedEntryPointCount(); ++i)
    {
        Slang::ComPtr<slang::IEntryPoint> entry_point;
        if (SLANG_SUCCEEDED(module->getDefinedEntryPoint(i, entry_point.writeRef())))
        {
            entry_points.Push(std::move(entry_point));
        }
    }

    if (entry_points.IsEmpty())
    {
        return Unexpected<ShaderCookError>{
            ShaderCookError::NoEntryPoint,
            String::Format("Shader has no entry point marked with [shader(...)]: {}", source_path),
            source_path,
        };
    }
    return entry_points;
}

/** 모듈과 모든 진입점을 하나의 프로그램으로 묶어 링크합니다. */
[[nodiscard]] ShaderCookResult<Slang::ComPtr<slang::IComponentType>> Link(
    slang::ISession* session,
    slang::IModule* module,
    const Array<Slang::ComPtr<slang::IEntryPoint>>& entry_points,
    const Path& source_path,
    String& diagnostics
)
{
    Array<slang::IComponentType*> components;
    components.Push(module);
    for (const Slang::ComPtr<slang::IEntryPoint>& entry_point : entry_points)
    {
        components.Push(entry_point.get());
    }

    Slang::ComPtr<slang::IBlob> diagnostic_blob;
    Slang::ComPtr<slang::IComponentType> composite;
    session->createCompositeComponentType(
        components.Data(), static_cast<SlangInt>(components.Len()), composite.writeRef(), diagnostic_blob.writeRef()
    );
    AppendDiagnostics(diagnostics, diagnostic_blob);
    if (!composite)
    {
        return Unexpected{ CompileError(source_path, "Failed to compose shader program", diagnostics) };
    }

    Slang::ComPtr<slang::IComponentType> linked;
    composite->link(linked.writeRef(), diagnostic_blob.writeRef());
    AppendDiagnostics(diagnostics, diagnostic_blob);
    if (!linked)
    {
        return Unexpected{ CompileError(source_path, "Failed to link shader program", diagnostics) };
    }
    return linked;
}

/** 진입점 하나의 포맷별 코드를 생성하고 리플렉션합니다. */
[[nodiscard]] ShaderCookResult<CompiledShaderStage> BuildStage(
    slang::IComponentType* linked,
    const SlangReflector& reflector,
    SlangUInt entry_index,
    const Path& source_path,
    String& diagnostics
)
{
    slang::EntryPointReflection* entry = linked->getLayout(TARGET_SPIRV)->getEntryPointByIndex(entry_index);
    const auto stage = ToShaderStage(entry->getStage());
    if (!stage)
    {
        return Unexpected<ShaderCookError>{
            ShaderCookError::NotSupported,
            String::Format("Entry point '{}' uses an unsupported stage: {}", entry->getName(), source_path),
            source_path,
        };
    }

    CompiledShaderStage compiled{ .stage = *stage };
    for (const TargetFormat& target_format : TARGET_FORMATS)
    {
        Slang::ComPtr<slang::IBlob> code;
        Slang::ComPtr<slang::IBlob> diagnostic_blob;
        linked->getEntryPointCode(static_cast<SlangInt>(entry_index), target_format.target, code.writeRef(), diagnostic_blob.writeRef());
        AppendDiagnostics(diagnostics, diagnostic_blob);
        if (!code)
        {
            return Unexpected{ CompileError(source_path, "Failed to generate shader code", diagnostics) };
        }

        compiled.blobs.Push({
            .stage = *stage,
            .format = target_format.format,
            .entry_point = entry->getName(),
            .code = ToBytes(code),
        });
    }

    compiled.stage_interface = reflector.ReflectInterface(entry_index, *stage);
    compiled.dxil_uniform_buffers = reflector.ReflectDxilUniformBuffers(*stage);
    compiled.inputs = reflector.ReflectInputs(entry_index);
    compiled.outputs = reflector.ReflectOutputs(entry_index);
    return compiled;
}

/** 소스 파일과 include한 파일 목록 */
[[nodiscard]] Array<Path> CollectDependencies(slang::IModule* module)
{
    Array<Path> dependencies;
    for (SlangInt32 i = 0; i < module->getDependencyFileCount(); ++i)
    {
        Path path = module->getDependencyFilePath(i);
        if (!dependencies.Contains(path))
        {
            dependencies.Push(std::move(path));
        }
    }
    return dependencies;
}
} // namespace

struct ShaderCompiler::Impl
{
    Slang::ComPtr<slang::IGlobalSession> global_session;
    String toolchain_identity;

    /** 컴파일 한 번에 쓸 세션을 만듭니다. 한 세션에서 같은 파일을 다시 불러오면 실패하므로 컴파일마다 새로 만듭니다. */
    [[nodiscard]] Slang::ComPtr<slang::ISession> CreateSession(const ShaderCompileRequest& request) const
    {
        const slang::TargetDesc targets[] = {
            { .format = SLANG_SPIRV, .profile = global_session->findProfile("spirv_1_0") },
            { .format = SLANG_DXIL,  .profile = global_session->findProfile("sm_6_0")    },
        };

        const slang::CompilerOptionEntry options[] = {
            EnableOption(slang::CompilerOptionName::VulkanUseEntryPointName),
            EnableOption(slang::CompilerOptionName::ForceDXLayout),
            {
                .name = slang::CompilerOptionName::DisableWarnings,
                .value = { .kind = slang::CompilerOptionValueKind::String, .stringValue0 = DISABLED_WARNINGS },
            },
        };

        Array<const char*> search_paths;
        for (const Path& include_dir : request.include_dirs)
        {
            search_paths.Push(include_dir.CStr());
        }

        slang::SessionDesc session_desc = {};
        session_desc.targets = targets;
        session_desc.targetCount = static_cast<SlangInt>(std::size(targets));
        session_desc.defaultMatrixLayoutMode = SLANG_MATRIX_LAYOUT_COLUMN_MAJOR;
        session_desc.searchPaths = search_paths.Data();
        session_desc.searchPathCount = static_cast<SlangInt>(search_paths.Len());
        session_desc.compilerOptionEntries = options;
        session_desc.compilerOptionEntryCount = static_cast<u32>(std::size(options));

        Slang::ComPtr<slang::ISession> session;
        global_session->createSession(session_desc, session.writeRef());
        return session;
    }

    [[nodiscard]] ShaderCookResult<CompiledShaderProgram> Compile(const ShaderCompileRequest& request) const
    {
        const Path& source_path = request.source_path;

        auto source = ReadSource(source_path);
        if (source.HasError())
        {
            return Unexpected{ std::move(source).Error() };
        }

        const Slang::ComPtr<slang::ISession> session = CreateSession(request);
        if (!session)
        {
            return Unexpected{ CompileError(source_path, "Failed to create Slang session", {}) };
        }

        CompiledShaderProgram program;
        Slang::ComPtr<slang::IBlob> diagnostic_blob;
        slang::IModule* module = session->loadModuleFromSourceString(
            MODULE_NAME, source_path.CStr(), source->CStr(), diagnostic_blob.writeRef()
        );
        AppendDiagnostics(program.diagnostics, diagnostic_blob);
        if (!module)
        {
            return Unexpected{ CompileError(source_path, "Failed to compile shader", program.diagnostics) };
        }

        auto entry_points = CollectEntryPoints(module, source_path);
        if (entry_points.HasError())
        {
            return Unexpected{ std::move(entry_points).Error() };
        }

        auto linked = Link(session, module, *entry_points, source_path, program.diagnostics);
        if (linked.HasError())
        {
            return Unexpected{ std::move(linked).Error() };
        }

        const SlangReflector reflector((*linked)->getLayout(TARGET_SPIRV), (*linked)->getLayout(TARGET_DXIL)); // NOLINT(*-redundant-parentheses)
        for (usize i = 0; i < entry_points->Len(); ++i)
        {
            auto stage = BuildStage(*linked, reflector, static_cast<SlangUInt>(i), source_path, program.diagnostics);
            if (stage.HasError())
            {
                return Unexpected{ std::move(stage).Error() };
            }
            program.stages.Push(std::move(stage).Value());
        }

        program.declared_bindings = reflector.GetDeclaredBindings();
        program.dependencies = CollectDependencies(module);
        return program;
    }
};

ShaderCompiler::ShaderCompiler()
    : impl(std::make_unique<Impl>())
{
    if (SLANG_FAILED(slang::createGlobalSession(impl->global_session.writeRef())))
    {
        return;
    }

    // Slang은 DXIL 생성을 DXC(dxcompiler.dll, 서명용 dxil.dll)에 맡기므로, 실행 파일 폴더에서 찾도록 지정합니다.
    const Path dxc_directory = Platform::GetExecutableDirectory();
    impl->global_session->setDownstreamCompilerPath(SLANG_PASS_THROUGH_DXC, dxc_directory.CStr());

    impl->toolchain_identity = String::Format(
        "Slang {}; {}; nowarn {}", impl->global_session->getBuildTagString(), FIXED_OPTIONS, DISABLED_WARNINGS
    );
}

bool ShaderCompiler::IsAvailable() const
{
    return impl->global_session != nullptr;
}

ShaderCookResult<CompiledShaderProgram> ShaderCompiler::Compile(const ShaderCompileRequest& request) const
{
    if (!IsAvailable())
    {
        return Unexpected<ShaderCookError>{
            ShaderCookError::NotSupported,
            String::Format("Slang compiler is not available: {}", request.source_path),
            request.source_path,
        };
    }
    return impl->Compile(request);
}

#else

struct ShaderCompiler::Impl
{
    String toolchain_identity;
};

ShaderCompiler::ShaderCompiler()
    : impl(std::make_unique<Impl>())
{
}

bool ShaderCompiler::IsAvailable() const
{
    return false;
}

ShaderCookResult<CompiledShaderProgram> ShaderCompiler::Compile(const ShaderCompileRequest& request) const
{
    return Unexpected<ShaderCookError>{
        ShaderCookError::NotSupported,
        String::Format("Shader compilation is not available on this platform: {}", request.source_path),
        request.source_path,
    };
}

#endif

ShaderCompiler::~ShaderCompiler() = default;

const String& ShaderCompiler::GetToolchainIdentity() const
{
    return impl->toolchain_identity;
}
} // namespace se::editor
