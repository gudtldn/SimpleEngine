#pragma once

#include "SimpleEditor/EditorCommon.h"
#include "SimpleEditor/ShaderCook/ShaderCookError.h"

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/Optional.h"
#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/Types/Path.h"
#include "SimpleEngine/Core/Types/StringName.h"
#include "SimpleEngine/Shader/ShaderBundle.h"

#include <memory>


namespace se::editor
{
/** 셰이더 소스에 선언된 리소스의 종류 */
enum class EShaderResourceKind : u8
{
    UniformBuffer,
    SampledTexture,
    Sampler,

    /** 읽기 전용 스토리지 리소스 */
    StorageTexture,
    StorageBuffer,

    /** 읽기/쓰기 스토리지 리소스. 컴퓨트 스테이지에만 존재 */
    ReadWriteStorageTexture,
    ReadWriteStorageBuffer,
};

/** 컴파일할 셰이더 소스 */
struct ShaderCompileRequest
{
    Path source_path;
    Array<Path> include_dirs;
};

/** 스테이지 사이를 오가는 보간 값 하나 */
struct ShaderVarying
{
    StringName name;
    String semantic; // 인덱스 포함, 예: TEXCOORD0
    EShaderValueType type = EShaderValueType::Unknown;
};

/** 스테이지 필터 전 소스에 선언된 리소스 하나 */
struct ShaderBindingRecord
{
    StringName name;
    EShaderResourceKind kind = EShaderResourceKind::UniformBuffer;
    u32 space = 0; // SPIR-V descriptor set
    u32 binding = 0;
};

/** 스테이지 하나의 컴파일 결과 */
struct CompiledShaderStage
{
    EShaderStage stage = EShaderStage::Vertex;

    /** 포맷마다 하나 */
    Array<ShaderBlob> blobs;

    /** SPIR-V 레이아웃 기준 계약 */
    ShaderStageInterface stage_interface;

    /** 포맷 간 오프셋 비교용 DXIL 레이아웃 상수 버퍼 */
    Array<ShaderUniformBuffer> dxil_uniform_buffers;

    /** 정점 스테이지는 정점 입력, 픽셀 스테이지는 보간 입력 */
    Array<ShaderVarying> inputs;

    /** 정점 스테이지의 보간 출력 */
    Array<ShaderVarying> outputs;
};

/** 셰이더 소스 하나의 컴파일 결과 */
struct SE_EDITOR_API CompiledShaderProgram
{
    Array<CompiledShaderStage> stages;
    Array<ShaderBindingRecord> declared_bindings;

    /** 소스 자신과 포함한 파일 */
    Array<Path> dependencies;

    /** 경고 원문 */
    String diagnostics;

    /** stage의 결과를 찾습니다. 그 스테이지가 없으면 NullOpt입니다. */
    [[nodiscard]] Optional<const CompiledShaderStage&> FindStage(EShaderStage stage) const;
};

/**
 * HLSL 호환 소스를 SPIR-V와 DXIL로 컴파일하고 SDL3 GPU 규약에 맞춘 계약을 만드는 컴파일러
 *
 * 내부 구현은 Slang이며, 공개 타입에는 Slang이 드러나지 않습니다.
 */
class SE_EDITOR_API ShaderCompiler
{
public:
    ShaderCompiler();
    ~ShaderCompiler();

    ShaderCompiler(const ShaderCompiler&) = delete;
    ShaderCompiler& operator=(const ShaderCompiler&) = delete;

    /** 이 플랫폼에서 컴파일할 수 있는지 여부 */
    [[nodiscard]] bool IsAvailable() const;

    /** request의 소스를 컴파일합니다. */
    [[nodiscard]] ShaderCookResult<CompiledShaderProgram> Compile(const ShaderCompileRequest& request) const;

    /** 컴파일러 버전과 고정 옵션을 담은 문자열. 캐시 키에 들어갑니다. */
    [[nodiscard]] const String& GetToolchainIdentity() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};
} // namespace se::editor
