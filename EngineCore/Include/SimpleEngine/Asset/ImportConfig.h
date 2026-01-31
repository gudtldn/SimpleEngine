#pragma once
#include <memory>

#include "SimpleEngine/Asset/ImportSettings/ImportSettings.h"
#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Reflection/TypeId.h"
#include "SimpleEngine/Utility/Debug.h"


namespace se::asset
{
/**
 * 에셋 임포트 파이프라인에서 사용되는 설정값들을 모아둔 클래스
 */
class SE_CORE_API ImportConfig
{
public:
    using SettingsMap = HashMap<refl::TypeId, std::shared_ptr<ImportSettingsBase>>;

public:
    /**
     * 특정 타입의 임포트 설정을 저장하거나 수정합니다.
     *
     * @tparam T ImportSettingsBase를 상속받은 구체적인 설정 클래스
     * @param settings 저장할 설정 객체
     */
    template <typename T>
        requires std::derived_from<T, ImportSettingsBase>
    void Set(const T& settings)
    {
        // TODO: 나중에 Asset Registry 만들면 Metadata에 옮겨야 할듯

        // 내부적으로 복사본을 만들어 shared_ptr로 관리
        settings_map.Insert(
            refl::TypeId::Get<T>(),
            std::make_shared<T>(settings)
        );
    }

    /**
     * 특정 타입의 임포트 설정이 존재하는지 확인하고, 있다면 반환합니다.
     *
     * @tparam T 조회할 설정 클래스 타입
     * @return 설정이 존재하면 해당 객체의 참조를 담은 Optional, 없으면 nullopt
     */
    template <typename T>
        requires std::derived_from<T, ImportSettingsBase>
    [[nodiscard]] Optional<const T&> Get() const
    {
        return settings_map
            .Find(refl::TypeId::Get<T>())
            .AndThen([](const auto& ptr) -> Optional<const T&>
            {
                return static_cast<const T&>(*ptr);
            });
    }

    /**
     * 특정 타입의 임포트 설정을 반환하되, 없으면 기본값(Default)을 반환합니다.
     *
     * @tparam T 조회할 설정 클래스 타입
     * @return 저장된 설정값 혹은 T의 기본 생성 객체
     */
    template <typename T>
        requires std::derived_from<T, ImportSettingsBase>
    [[nodiscard]] T GetOrDefault() const
    {
        return Get<T>().Copy().ValueOrDefault();
    }

    friend void Serialize([[maybe_unused]] Archive& ar, [[maybe_unused]] ImportConfig& config)
    {
        // TODO: ImportConfig Serialize 구현
        SE_UNIMPLEMENTED();
    }

private:
    SettingsMap settings_map;
};
}  // namespace se::asset
