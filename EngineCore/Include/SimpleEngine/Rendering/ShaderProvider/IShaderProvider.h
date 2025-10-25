#pragma once
#include <filesystem>
#include <utility>

#include "SimpleEngine/Core/Containers/Containers.h"
#include "SimpleEngine/Core/Container/Optional.h"
#include "SimpleEngine/Utility/Hash.h"

#include "SDL3/SDL_gpu.h"


namespace se::rendering
{
/**
 * Shader 요청에 필요한 정보
 */
struct ShaderRequest
{
    std::filesystem::path source_path;

    // HLSL 컴파일 시 사용
    Optional<std::filesystem::path> hlsl_include_dir_opt = std::nullopt;
    Optional<vector<std::pair<const char*, const char*>>> hlsl_defines_opt = std::nullopt;

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
}


template <>
struct std::hash<se::rendering::ShaderRequest>
{
    size_t operator()(const se::rendering::ShaderRequest& request) const noexcept
    {
        using se::utility::HashCombine;

        size_t seed = 0;

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
                HashCombine(seed, std::string_view{ name });
                HashCombine(seed, std::string_view{ value });
            }
        }

        return seed;
    }
};
