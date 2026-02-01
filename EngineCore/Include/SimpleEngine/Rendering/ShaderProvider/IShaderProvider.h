#pragma once
#include <algorithm>
#include <utility>

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/Optional.h"
#include "SimpleEngine/Core/Types/Path.h"
#include "SimpleEngine/Utility/Hash.h"

#include "SDL3/SDL_gpu.h"


namespace se::graphics
{
/**
 * Shader 요청에 필요한 정보
 */
struct ShaderRequest
{
    Path source_path;

    // HLSL 컴파일 시 사용
    Optional<Path> hlsl_include_dir_opt = std::nullopt;
    Optional<Array<std::pair<const char*, const char*>>> hlsl_defines_opt = std::nullopt;

    bool operator==(const ShaderRequest& other) const = default;
};

/**
 * ShaderManager에 SDL_GPUShader* 를 제공하는 인터페이스
 */
class IShaderProvider
{
public:
    virtual ~IShaderProvider() = default;

    /** 주어진 Request에 따라 Shader를 가져옵니다. */
    virtual SDL_GPUShader* Provide(SDL_GPUDevice* device, const ShaderRequest& request) = 0;
};
}  // namespace se::graphics


template <>
struct std::hash<se::graphics::ShaderRequest>
{
    // ReSharper disable once CppDFAConstantFunctionResult
    size_t operator()(const se::graphics::ShaderRequest& request) const noexcept
    {
        using se::utility::HashCombine;

        usize seed = 0;

        HashCombine(seed, request.source_path);
        if (request.hlsl_include_dir_opt.HasValue())
        {
            HashCombine(seed, *request.hlsl_include_dir_opt);
        }

        if (request.hlsl_defines_opt.HasValue())
        {
            auto defines = request.hlsl_defines_opt.Value();

            std::ranges::sort(defines, [](const auto& a, const auto& b) -> bool
            {
                return a.first < b.first;
            });

            for (const auto& [name, value] : defines)
            {
                HashCombine(seed, se::StringView{ name });
                HashCombine(seed, se::StringView{ value });
            }
        }

        return static_cast<size_t>(seed);
    }
};
