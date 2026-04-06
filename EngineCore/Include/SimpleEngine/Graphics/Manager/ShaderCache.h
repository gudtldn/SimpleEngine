#pragma once

#include "SimpleEngine/Core/Container/ArrayView.h"
#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Types/VPath.h"
#include "SimpleEngine/Core/Types/Path.h"

#include "SDL3/SDL_gpu.h"
#include "SDL3_shadercross/SDL_shadercross.h"


namespace se::graphics
{
// forward declaration
class RenderDevice;

/**
 * VPath 기반 셰이더 캐시
 * 논리 키(예: "CoreShader://DebugLine.vert")를 받아 내부적으로 .spv 경로를 resolve합니다.
 */
class SE_CORE_API ShaderCache
{
public:
    explicit ShaderCache(RenderDevice& in_render_device);
    ~ShaderCache();

    // 이동만 가능
    ShaderCache(const ShaderCache&) = delete;
    ShaderCache& operator=(const ShaderCache&) = delete;
    ShaderCache(ShaderCache&&) = default;
    ShaderCache& operator=(ShaderCache&&) = default;

public:
    // ---- Graphics Shader (vertex / fragment) ----

    /** VPath 키로 SDL_GPUShader*를 가져오거나 .spv에서 새로 로드합니다. */
    [[nodiscard]] SDL_GPUShader* GetOrCreateShader(const VPath& shader_key, SDL_ShaderCross_ShaderStage stage);

    /** SPIR-V 바이트를 메모리에서 직접 Graphics 셰이더로 생성/교체합니다. */
    void LoadShaderFromMemory(const VPath& shader_key, SDL_ShaderCross_ShaderStage stage, ArrayView<const uint8> spirv_bytecode);


    // ---- Compute Pipeline ----

    /** VPath 키로 SDL_GPUComputePipeline*를 가져오거나 .spv에서 새로 로드합니다. */
    [[nodiscard]] SDL_GPUComputePipeline* GetOrCreateComputePipeline(const VPath& shader_key);

    /** SPIR-V 바이트를 메모리에서 직접 Compute 파이프라인으로 생성/교체합니다. */
    void LoadComputePipelineFromMemory(const VPath& shader_key, ArrayView<const uint8> spirv_bytecode);


    // ---- Common ----

    /** 특정 셰이더를 캐시에서 제거하고 GPU 리소스를 해제합니다. (Graphics + Compute 모두 확인) */
    void Invalidate(const VPath& shader_key);

    /** 캐시를 모두 해제 후 삭제합니다. */
    void ClearAll();

    /** VPath 키를 .spv 물리 경로로 resolve합니다. */
    [[nodiscard]] static Path ResolveSpvPath(const VPath& shader_key);

private:
    /** .spv 물리 경로에서 SPIR-V 바이트를 읽어 반환합니다. */
    [[nodiscard]] static Optional<Array<uint8>> ReadSpvFile(const VPath& shader_key);

    RenderDevice* render_device;
    HashMap<VPath, SDL_GPUShader*> graphics_cache;
    HashMap<VPath, SDL_GPUComputePipeline*> compute_cache;
};
} // namespace se::graphics
