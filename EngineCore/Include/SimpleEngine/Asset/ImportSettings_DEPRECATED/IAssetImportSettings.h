#pragma once
#include <memory>

#include "SimpleEngine/Core/Serialization/Archive.h"
#include "SimpleEngine/Core/Reflection/TypeId.h"


namespace se::asset
{
/**
 * AssetImportSettings의 기본 인터페이스
 * 모든 AssetImportSettings 클래스는 이를 상속받아야 합니다.
 */
class SE_CORE_API IAssetImportSettings
{
public:
    virtual ~IAssetImportSettings() = default;

    /** 현재 ImportSettings를 깊은 복사 합니다. */
    [[nodiscard]] virtual std::shared_ptr<IAssetImportSettings> Clone() const = 0;

    /** 현재 ImportSettings의 TypeId를 반환합니다. */
    [[nodiscard]] virtual TypeId GetTypeId() const = 0;

    /** ImportSettings를 Archive로 직렬화합니다. */
    virtual void Serialize(Archive& ar) = 0;

public:
    friend void Serialize(Archive& ar, IAssetImportSettings& settings)
    {
        settings.Serialize(ar);
    }
};

template <typename Derived>
class AssetImportSettingsBase : public IAssetImportSettings
{
protected:
    AssetImportSettingsBase() = default;

public:
    [[nodiscard]] virtual std::shared_ptr<IAssetImportSettings> Clone() const override
    {
        return std::make_shared<Derived>(static_cast<const Derived&>(*this));
    }

    [[nodiscard]] virtual TypeId GetTypeId() const override final
    {
        return TypeId::Get<Derived>();
    }
};
}  // namespace se::asset
