export module SE.Assets:Handle;

import SE.Types;
import std;


export namespace se::assets
{
/**
 *
 * @tparam T
 */
template <typename T>
struct Handle
{
    using AssetType = T;
    StringName asset_id = StringName::None;

    [[nodiscard]] bool operator==(const Handle&) const = default;
    [[nodiscard]] auto operator<=>(const Handle&) const = default;

    [[nodiscard]] bool IsValid() const noexcept { return asset_id != StringName::None; }
    [[nodiscard]] explicit operator bool() const noexcept { return IsValid(); }
};
}
