#include "SimpleEditor/ShaderCook/ShaderCooker.h"

#include "SimpleEditor/ShaderCook/ShaderCompiler.h"

#include "SimpleEngine/Asset/DerivedDataCache.h"
#include "SimpleEngine/Core/FileSystem/FileSystem.h"
#include "SimpleEngine/Core/FileSystem/VFS.h"
#include "SimpleEngine/Core/Logging/Logging.h"
#include "SimpleEngine/Utility/SHA256.h"

#include <algorithm>


namespace se::editor
{
namespace
{
/** 모든 셰이더가 공통 헤더를 찾는 include 경로 */
constexpr StringView SHADER_INCLUDE_DIR = "CoreShader://";

/** 쿡할 셰이더 소스 확장자 */
constexpr const char* SHADER_EXTENSION = ".hlsl";

/** 검증 문제를 한 줄에 하나씩 이어 붙입니다. */
[[nodiscard]] String JoinLines(const Array<String>& lines)
{
    String joined;
    for (const String& line : lines)
    {
        if (!joined.IsEmpty())
        {
            joined += '\n';
        }
        joined += line;
    }
    return joined;
}

/** 컴파일러가 준 의존 파일 경로를 정렬된 VPath 문자열로 바꿉니다. */
[[nodiscard]] ShaderCookResult<Array<String>> ToVPathStrings(const Array<Path>& dependencies, const Path& source_path)
{
    Array<String> vpaths;
    for (const Path& dependency : dependencies)
    {
        const auto vpath = VFS::Unresolve(dependency);
        if (!vpath)
        {
            return Unexpected<ShaderCookError>{
                ShaderCookError::CompileFailed,
                String::Format("Shader dependency is outside the mounted directories: {}", dependency),
                source_path,
            };
        }
        vpaths.Push(vpath->ToString());
    }

    // 컴파일러가 의존 파일을 주는 순서와 관계없이 번들과 해시가 같도록 정렬합니다.
    std::ranges::sort(vpaths);
    return vpaths;
}

/** 스테이지별 컴파일 결과를 번들 하나로 모읍니다. */
[[nodiscard]] ShaderBundle MakeBundle(CompiledShaderProgram&& compiled, Array<String>&& dependencies)
{
    ShaderBundle bundle;
    for (CompiledShaderStage& stage : compiled.stages)
    {
        bundle.program.stages.Push(std::move(stage.stage_interface));
        for (ShaderBlob& blob : stage.blobs)
        {
            bundle.blobs.Push(std::move(blob));
        }
    }
    bundle.dependencies = std::move(dependencies);
    return bundle;
}
} // namespace


ShaderCooker::ShaderCooker(const ShaderCompiler& compiler)
    : compiler(compiler)
{
}

ShaderCookSummary ShaderCooker::CookDirectory(const VPath& shader_dir, DerivedDataCache& ddc) const
{
    ShaderCookSummary summary;

    const Path directory = VFS::ToPath(shader_dir);
    if (!fs::Exists(directory))
    {
        ConsoleLog(ELogLevel::Warning, "Shader directory does not exist: {}", shader_dir.ToString());
        return summary;
    }

    for (const DirectoryEntry& entry : fs::ReadDir(directory))
    {
        const Path& source_path = entry.GetPath();
        const auto file_name = source_path.FileName();
        if (!entry.IsFile() || source_path.Extension() != SHADER_EXTENSION || !file_name)
        {
            continue;
        }

        const VPath shader_vpath = shader_dir / *file_name;
        const Guid key = BundleKeyOf(shader_vpath);
        if (IsUpToDate(key, ddc))
        {
            ++summary.up_to_date;
            continue;
        }

        auto bundle = CookFile(shader_vpath);
        if (!bundle)
        {
            ConsoleLog(ELogLevel::Error, "Shader cook failed: {}\n{}", shader_vpath.ToString(), bundle.Error().What());
            ++summary.failed;
            continue;
        }

        const auto source_hash = HashSources(bundle->dependencies);
        if (!source_hash || !ddc.Store(key, CacheEntry{ .source_hash = *source_hash, .cache_version = ShaderBundle::FORMAT_VERSION, .payload = bundle->Serialize() }))
        {
            ConsoleLog(ELogLevel::Error, "Failed to store shader bundle: {}", shader_vpath.ToString());
            ++summary.failed;
            continue;
        }
        ++summary.cooked;
    }
    return summary;
}

ShaderCookResult<ShaderBundle> ShaderCooker::CookFile(const VPath& shader_vpath) const
{
    const ShaderCompileRequest request{
        .source_path = VFS::ToPath(shader_vpath),
        .include_dirs = { VFS::ToPath(VPath{ SHADER_INCLUDE_DIR }) },
    };

    auto compiled = compiler.Compile(request);
    if (!compiled)
    {
        return Unexpected{ std::move(compiled).Error() };
    }
    if (!compiled->diagnostics.IsEmpty())
    {
        ConsoleLog(ELogLevel::Warning, "Shader compiler warnings: {}\n{}", shader_vpath.ToString(), compiled->diagnostics);
    }

    const Array<String> problems = compiled->Validate();
    if (!problems.IsEmpty())
    {
        return Unexpected<ShaderCookError>{ ShaderCookError::ValidationFailed, JoinLines(problems), request.source_path };
    }

    auto dependencies = ToVPathStrings(compiled->dependencies, request.source_path);
    if (!dependencies)
    {
        return Unexpected{ std::move(dependencies).Error() };
    }
    return MakeBundle(std::move(*compiled), std::move(*dependencies));
}

Guid ShaderCooker::BundleKeyOf(const VPath& shader_vpath)
{
    const ContentHash hash = sha256::HashString(String::Format("ShaderBundle:{}", shader_vpath.ToString()));

    FixedArray<u8, 16> bytes{};
    std::copy_n(hash.Data(), bytes.Len(), bytes.Data());

    // 이름 기반 UUID 버전 8(RFC 9562)과 RFC 변형 비트
    bytes[6] = static_cast<u8>((bytes[6] & 0x0F) | 0x80); // NOLINT(*-signed-bitwise)
    bytes[8] = static_cast<u8>((bytes[8] & 0x3F) | 0x80); // NOLINT(*-signed-bitwise)
    return Guid::FromBytes(bytes);
}

bool ShaderCooker::IsUpToDate(const Guid& key, const DerivedDataCache& ddc) const
{
    // 처음 쿡하는 셰이더에서 Load가 읽기 실패 경고를 남기지 않도록 먼저 확인합니다.
    if (!ddc.Contains(key))
    {
        return false;
    }

    const auto entry = ddc.Load(key);
    if (!entry || entry->cache_version != ShaderBundle::FORMAT_VERSION)
    {
        return false;
    }

    const auto bundle = ShaderBundle::Deserialize(entry->payload);
    if (!bundle)
    {
        return false;
    }

    const auto source_hash = HashSources(bundle->dependencies);
    return source_hash && *source_hash == entry->source_hash;
}

Optional<ContentHash> ShaderCooker::HashSources(const Array<String>& dependencies) const
{
    String text;
    for (const String& dependency : dependencies)
    {
        const auto path = VFS::Resolve(VPath{ dependency });
        if (!path)
        {
            return NullOpt;
        }
        text += String::Format("{}:{}\n", dependency, sha256::HashFile(*path).ToHex());
    }
    text += compiler.GetToolchainIdentity();
    return sha256::HashString(text);
}
} // namespace se::editor
