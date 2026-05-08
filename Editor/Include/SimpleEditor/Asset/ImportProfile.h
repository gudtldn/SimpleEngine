#pragma once

#include "SimpleEditor/EditorCommon.h"
#include "SimpleEditor/Asset/ImportSettings/ImportSettingsBase.h"

#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Reflection/TypeId.h"
#include "SimpleEngine/Core/Reflection/TypeRegistry.h"

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
    using SettingsMap = HashMap<TypeId, std::shared_ptr<ImportSettingsBase>>;

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
            TypeId::Get<PureType>(),
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
            TypeId::Get<T>(),
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
            .Find(TypeId::Get<T>())
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

public:
    /**
     * ImportSettingsBase 파생 객체를 TypeId 기반으로 직렬화/역직렬화합니다.
     *
     * 직렬화(Save): 등록된 각 설정의 TypeId를 키로 사용하여 serialize 합니다.
     * 역직렬화(Load): TypeId로 TypeRegistry에서 TypeInfo를 찾고, constructor로 인스턴스를 생성한 뒤 deserialize 합니다.
     */
    friend void Serialize(Archive& ar, ImportProfile& config)
    {
        if (ar.IsLoading())
        {
            uint64 count = 0;
            ar.BeginMap(count);
            for (uint64 i = 0; i < count; ++i)
            {
                ar.BeginMapKey();
                TypeId type_id;
                ar << type_id;
                ar.EndMapKey();

                const auto& registry = TypeRegistry::Get();
                const auto info_opt = registry.Find(type_id);

                ar.BeginMapValue();
                if (info_opt.HasValue() && info_opt->constructor)
                {
                    void* raw = info_opt->constructor();
                    ImportSettingsBase* settings = static_cast<ImportSettingsBase*>(raw);

                    ar.BeginObject();
                    if (info_opt->serialize)
                    {
                        info_opt->serialize(ar, settings);
                    }
                    ar.EndObject();

                    config.settings_map.Insert(type_id, std::shared_ptr<ImportSettingsBase>(settings));
                }
                ar.EndMapValue();
            }
            ar.EndMap();
        }
        else
        {
            uint64 count = config.settings_map.Len();
            ar.BeginMap(count);
            for (auto& [type_id, settings_ptr] : config.settings_map)
            {
                ar.BeginMapKey();
                ar << type_id;
                ar.EndMapKey();

                ar.BeginMapValue();
                const auto& registry = TypeRegistry::Get();
                const auto info_opt = registry.Find(type_id);
                if (info_opt && info_opt->serialize && settings_ptr)
                {
                    ar.BeginObject();
                    info_opt->serialize(ar, settings_ptr.get());
                    ar.EndObject();
                }
                ar.EndMapValue();
            }
            ar.EndMap();
        }
    }

private:
    SettingsMap settings_map;
};
} // namespace se::editor
