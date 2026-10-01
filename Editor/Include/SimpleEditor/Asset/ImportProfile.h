#pragma once

#include "SimpleEditor/EditorCommon.h"
#include "SimpleEditor/Asset/ImportSettings/ImportSettingsBase.h"

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/Error/Expected.h"
#include "../../../../EngineCore/Include/SimpleEngine/Core/Reflection/Legacy/TypeId.h"
#include "SimpleEngine/Core/Reflection//Legacy/TypeRegistry.h"
#include "SimpleEngine/Core/Reflection/Registrar.h"
#include "SimpleEngine/Core/Serialization/SerializeTraits.h"
#include "SimpleEngine/Core/Types/HashDigest.h"

#include <memory>


namespace se::editor
{
/**
 * 에셋 임포트 파이프라인에서 사용되는 설정값들을 모아둔 클래스
 *
 * TypeId를 키로 하여 다양한 ImportSettingsBase 파생 객체를 보관합니다.
 * 리플렉션 기반 직렬화를 지원하여 .meta 파일의 import_settings 섹션에 사용할 수 있습니다.
 */
class SE_EDITOR_API ImportProfile
{
public:
    using SettingsMap = HashMap<TypeId_v1, std::shared_ptr<ImportSettingsBase>>;

public:
    /**
     * 특정 타입의 임포트 설정을 저장하거나 수정합니다.
     *
     * @tparam T ImportSettingsBase를 상속받은 구체적인 설정 클래스
     * @param settings 저장할 설정 객체
     */
    template <typename T>
        requires std::derived_from<std::remove_cvref_t<T>, ImportSettingsBase>
    void Set(T&& settings)
    {
        using PureType = std::remove_cvref_t<T>;
        settings_map.Insert(
            TypeId_v1::Of<PureType>(),
            std::make_shared<PureType>(std::forward<T>(settings))
        );
    }

    /**
     * 특정 타입의 임포트 설정을 In-place로 생성하여 저장합니다.
     *
     * @tparam T ImportSettingsBase를 상속받은 구체적인 설정 클래스
     * @param args 생성자에 전달할 인자
     */
    template <typename T, typename... Args>
        requires std::derived_from<T, ImportSettingsBase>
    void Emplace(Args&&... args)
    {
        settings_map.Emplace(
            TypeId_v1::Of<T>(),
            std::make_shared<T>(std::forward<Args>(args)...)
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
            .Find(TypeId_v1::Of<T>())
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

    /** SettingsMap에 접근하는 Getter입니다. */
    [[nodiscard]] FORCE_INLINE const SettingsMap& GetSettingsMap() const { return settings_map; }

    /**
     * 설정이 바뀌었는지 비교할 SHA-256 해시를 계산합니다.
     * 설정을 Packed로 쓴 바이트의 해시이고, 설정 타입 이름 순으로 쓰므로 설정을 넣은 순서와 무관합니다.
     * 설정을 직렬화하지 못하면 오류 메시지를 돌려줍니다.
     */
    [[nodiscard]] Expected<ContentHash, String> ComputeHash() const;

public:
    /** 새 직렬화의 트레이트(ImportProfile.cpp)가 읽은 설정을 settings_map에 넣습니다. */
    friend struct SerializeTraits<ImportProfile>;

private:
    SettingsMap settings_map;
};

/**
 * ImportProfile을 읽다가 건너뛴 설정 타입의 이름을 모읍니다.
 * 로더가 SerializeContext에 넣어 두면 모르는 설정 타입을 건너뛴 일을 알릴 수 있고, 넣지 않으면 기록 없이 건너뜁니다.
 */
struct SkippedImportSettings
{
    Array<String> type_names;
};
} // namespace se::editor

// 설정 타입이 런타임에 정해지는 다형 맵이라 필드로 서술할 수 없으므로 Opaque로 등록하고, 직렬화는 ImportProfile.cpp의 SerializeTraits가 맡음
SE_REFLECT_OPAQUE(se::editor::ImportProfile)
