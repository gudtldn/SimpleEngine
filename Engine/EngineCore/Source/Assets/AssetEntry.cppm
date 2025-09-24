export module SE.Assets:AssetEntry;

import SE.Types;
import std;

export namespace se::assets
{
/**
 * 에셋의 현재 로딩 상태를 나타냅니다.
 */
enum class EAssetState : uint8
{
    NotLoaded, // 로드된 적 없음
    Loading,   // 현재 로딩 중
    Loaded,    // 로딩 성공
    Failed     // 로딩 실패
};

/**
 * 각 에셋의 메타데이터와 실제 데이터를 관리하는 내부 구조체
 * @tparam T 에셋 타입
 */
template <typename T>
struct AssetEntry
{
    // Asset의 로딩 상태
    std::atomic<EAssetState> state = EAssetState::NotLoaded;

    // 로딩이 완료된 실제 에셋 데이터
    std::shared_ptr<T> asset_data = nullptr;

    // 로딩이 진행 중일 때, 다른 요청자들이 결과를 기다릴 수 있도록 하는 future.
    std::shared_future<std::shared_ptr<T>> future;
};
}
