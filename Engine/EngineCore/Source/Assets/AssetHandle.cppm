export module SE.Assets:AssetHandle;

import SE.Types;
import std;


export namespace se::assets
{
/**
 * AssetManager가 관리하는 에셋에 대한 가볍고 안전한 비소유(non-owning) Handle
 * @tparam T 참조할 에셋의 타입
 */
template <typename T>
struct AssetHandle
{
    using AssetType = T;
    StringName asset_id = StringName::None;

    [[nodiscard]] bool operator==(const AssetHandle&) const = default;
    [[nodiscard]] auto operator<=>(const AssetHandle&) const = default;

    [[nodiscard]] bool IsValid() const noexcept { return asset_id != StringName::None; }
    [[nodiscard]] explicit operator bool() const noexcept { return IsValid(); }
};
}
