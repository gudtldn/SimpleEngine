#pragma once
#include <concepts>

#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Core/Reflection/TypeId.h"


namespace se::asset
{
/**
 * Asset을 나타내는 가장 기본적인 타입
 */
class SE_CORE_API IAsset
{
public:
    virtual ~IAsset() = default;

    /** 이 Asset의 타입 ID를 반환합니다. */
    [[nodiscard]] virtual TypeId GetAssetType() const = 0;

    /** 이 Asset의 표시 이름을 반환합니다. (디버깅/에디터용) */
    [[nodiscard]] virtual StringView GetDisplayName() const { return {}; }
};

/**
 * CRTP 패턴을 활용한 Asset 기본 템플릿 클래스
 * @tparam Derived 실제 이 클래스를 상속받는 하위 클래스 타입
 */
template <typename Derived>
class Asset : public IAsset
{
protected:
    Asset() = default;

public:
    [[nodiscard]] virtual TypeId GetAssetType() const override final
    {
        static_assert(std::derived_from<Derived, Asset>, "CRTP Error: Derived class must inherit from Asset<Derived>");
        return TypeId::Get<Derived>();
    }
};
}  // namespace se::asset
