export module SE.Assets:AssetEntry;

import SE.Types;
import std;


namespace se::assets
{
/**
 * 에셋의 현재 로딩 상태를 나타냅니다.
 */
enum class EAssetState : uint8
{
    Unloaded, // 로드된 적이 없거나, 해제됨
    Loading,  // 현재 로딩 중
    Loaded,   // 로딩 성공
    Failed    // 로딩 실패
};

/**
 * 각 에셋의 메타데이터와 실제 데이터를 관리하는 내부 구조체
 * @tparam T 에셋 타입
 */
template <typename T>
struct AssetEntry
{
    // Asset의 로딩 상태
    std::atomic<EAssetState> state = EAssetState::Unloaded;

    // 추후 실제 에셋 데이터가 담길 future
    std::shared_future<std::shared_ptr<T>> future;
};
}
