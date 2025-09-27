export module SE.Core:Paths;

export import :Paths.PathResolver;
import std;


export namespace se::core::paths
{
/**
 * VPath를 실제 물리적 경로로 해석합니다.
 * @param virtual_path 해석할 가상 경로
 * @return 해당하는 물리적 경로. 유효하지 않으면 비어있는 경로를 반환합니다.
 */
[[nodiscard]] Optional<std::filesystem::path> Resolve(const VPath& virtual_path)
{
    return PathResolver::Get().Resolve(virtual_path);
}

/**
 * 물리적 경로를 가장 적합한 VPath로 역해석합니다.
 * @param physical_path 역해석할 물리적 경로
 * @return 해당하는 가상 경로. 유효하지 않으면 비어있는 경로를 반환합니다.
 */
[[nodiscard]] Optional<VPath> Unresolve(const std::filesystem::path& physical_path)
{
    return PathResolver::Get().Unresolve(physical_path);
}
}
