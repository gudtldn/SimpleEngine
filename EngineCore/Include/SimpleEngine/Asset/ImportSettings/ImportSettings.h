#pragma once
#include <concepts>

#include "SimpleEngine/Core/Serialization/Archive.h"
#include "SimpleEngine/Reflection/TypeId.h"


namespace se::asset
{
/**
 * 에셋 임포트 설정(Import Settings)의 기본 클래스
 */
class SE_CORE_API ImportSettingsBase
{
public:
    virtual ~ImportSettingsBase() = default;

    /** 현재 ImportSettings의 TypeId를 반환합니다. */
    [[nodiscard]] virtual refl::TypeId GetTypeId() const = 0;

    /** ImportSettings를 Archive로 직렬화합니다. */
    virtual void Serialize(core::Archive& ar) = 0;

public:
    friend void Serialize(core::Archive& ar, ImportSettingsBase& settings)
    {
        settings.Serialize(ar);
    }
};

/**
 * 에셋 임포트 설정 클래스를 정의할 때 사용하는 CRTP 기반 헬퍼 클래스
 * @tparam Derived ImportSettings를 상속받는 클래스
 */
template <typename Derived>
class ImportSettings : public ImportSettingsBase
{
    friend Derived;

protected:
    ImportSettings() = default;
    virtual ~ImportSettings() override = default;

public:
    [[nodiscard]] virtual refl::TypeId GetTypeId() const override final
    {
        static_assert(std::derived_from<Derived, ImportSettings>, "CRTP Error: Derived class must inherit from ImportSettings<Derived>");
        return refl::TypeId::Get<Derived>();
    }
};
}
