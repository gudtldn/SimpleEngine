#pragma once

#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Container/Optional.h"
#include "SimpleEngine/Core/Types/VPath.h"
#include "SimpleEngine/Shader/ShaderBundle.h"

#include "SDL3/SDL_gpu.h"


namespace se
{
// forward declaration
class IShaderBundleSource;

/**
 * 쿡된 셰이더 번들로 SDL GPU 셰이더와 컴퓨트 파이프라인을 만드는 라이브러리
 * 프로그램(셰이더 소스 파일 하나)마다 번들을 한 번 읽고, 스테이지별 셰이더를 캐시합니다.
 */
class SE_CORE_API ShaderLibrary
{
public:
    ShaderLibrary(SDL_GPUDevice* in_device, const IShaderBundleSource& in_source);
    ~ShaderLibrary();

    ShaderLibrary(const ShaderLibrary&) = delete;
    ShaderLibrary& operator=(const ShaderLibrary&) = delete;
    ShaderLibrary(ShaderLibrary&&) = delete;
    ShaderLibrary& operator=(ShaderLibrary&&) = delete;

public:
    /** program의 stage 셰이더를 가져오거나 번들에서 만듭니다. 실패하면 로그를 남기고 nullptr */
    [[nodiscard]] SDL_GPUShader* GetOrCreateShader(const VPath& program, EShaderStage stage);

    /**
     * 컴퓨트 프로그램으로 파이프라인을 새로 만듭니다. 실패하면 로그를 남기고 nullptr
     * @note 캐시하지 않으므로 해제는 호출한 쪽이 합니다.
     */
    [[nodiscard]] SDL_GPUComputePipeline* CreateComputePipeline(const VPath& program, SDL_PropertiesID props = 0);

    /** 이미 읽은 프로그램의 인터페이스. 읽은 적이 없으면 NullOpt */
    [[nodiscard]] Optional<const ShaderProgramInterface&> FindInterface(const VPath& program) const;

    /** 만든 셰이더를 모두 해제하고 읽은 번들을 비웁니다. */
    void ClearAll();

public:
    /** 장치가 받는 포맷 중 DXIL, SPIR-V 순으로 stage의 블롭을 고릅니다. 맞는 블롭이 없으면 NullOpt */
    [[nodiscard]] static Optional<const ShaderBlob&> ChooseBlob(
        const ShaderBundle& bundle,
        EShaderStage stage,
        SDL_GPUShaderFormat device_formats
    );

    /**
     * 그래픽스 셰이더 생성 정보
     * @note 코드와 진입점은 blob을 가리키므로 blob이 살아 있는 동안만 씁니다.
     */
    [[nodiscard]] static SDL_GPUShaderCreateInfo MakeShaderCreateInfo(
        const ShaderStageInterface& stage_interface,
        const ShaderBlob& blob
    );

    /**
     * 컴퓨트 파이프라인 생성 정보
     * @note 코드와 진입점은 blob을 가리키므로 blob이 살아 있는 동안만 씁니다.
     */
    [[nodiscard]] static SDL_GPUComputePipelineCreateInfo MakeComputePipelineCreateInfo(
        const ShaderStageInterface& stage_interface,
        const ShaderBlob& blob
    );

private:
    /** 읽은 번들과 그 번들로 만든 그래픽스 셰이더 */
    struct LoadedProgram
    {
        ShaderBundle bundle;
        SDL_GPUShader* vertex_shader = nullptr;
        SDL_GPUShader* fragment_shader = nullptr;
    };

    /** 읽은 프로그램을 찾고, 없으면 번들 소스에서 읽습니다. 실패하면 로그를 남기고 NullOpt */
    [[nodiscard]] Optional<LoadedProgram&> FindOrLoad(const VPath& program);

    /** bundle의 stage 셰이더를 만듭니다. 실패하면 로그를 남기고 nullptr */
    [[nodiscard]] SDL_GPUShader* CreateShader(const VPath& program, const ShaderBundle& bundle, EShaderStage stage) const;

private:
    SDL_GPUDevice* device;
    const IShaderBundleSource& source;
    HashMap<VPath, LoadedProgram> programs;
};
} // namespace se
