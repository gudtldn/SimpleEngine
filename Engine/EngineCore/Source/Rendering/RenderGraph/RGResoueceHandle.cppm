export module SE.Rendering:RenderGraph.RGResoueceHandle;

import SE.Types;
import std;


export namespace se::rendering::render_graph
{
/**
 * Render Graph 내의 리소스를 가리키는 핸들
 */
struct RGResourceHandle
{
    size_t index = std::numeric_limits<size_t>::max();

    [[nodiscard]] constexpr bool operator==(const RGResourceHandle&) const = default;
    [[nodiscard]] constexpr auto operator<=>(const RGResourceHandle&) const = default;

    [[nodiscard]] constexpr bool IsValid() const { return index != std::numeric_limits<size_t>::max(); }
    [[nodiscard]] explicit constexpr operator bool() const { return IsValid(); }
};
}
