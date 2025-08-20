export module SimpleEngine.Rendering:RenderGraph.RGResoueceHandle;

import SimpleEngine.Types;
import std;


export namespace se::rendering::render_graph
{
/**
 * Render Graph 내의 리소스를 가리키는 핸들
 */
struct RGResourceHandle
{
    size_t index = static_cast<size_t>(-1);

    [[nodiscard]] explicit operator bool() const noexcept { return index != static_cast<size_t>(-1); }
    [[nodiscard]] bool operator==(const RGResourceHandle&) const = default;
    [[nodiscard]] auto operator<=>(const RGResourceHandle&) const = default;
};
}
