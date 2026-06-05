#pragma once

#include "SimpleEditor/EditorCommon.h"
#include "SimpleEditor/Asset/ImportProfile.h"

#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Functional/Function.h"
#include "SimpleEngine/Core/Reflection/TypeId.h"

#include <concepts>


namespace se::editor
{
// forward declaration
class IPipelineTranslator;

/**
 * Translator 타입별 기본 ImportProfile을 관리하는 매니저
 *
 * 각 Translator(예: AssimpTranslator)에 대해 기본 ImportSettings를 미리 등록해 두면,
 * EnsureMetaFile()에서 새 .meta 파일 생성 시 빈 프로파일 대신 적절한 기본값을 사용할 수 있습니다.
 */
class SE_EDITOR_API ImportPresetManager
{
public:
    /**
     * Translator 타입에 대한 기본 프리셋 초기화 함수를 등록합니다.
     * @param translator_type Translator의 TypeId (예: TypeId::Of<AssimpTranslator>())
     * @param initializer ImportProfile에 기본 설정을 채우는 콜백
     */
    void RegisterPreset(const TypeId& translator_type, Function<void(ImportProfile&)> initializer);

    /**
     * 템플릿 인자 T를 통해 Translator 타입을 자동으로 추론하여 프리셋 초기화 함수를 등록합니다.
     * @tparam T 등록할 Translator 클래스 타입 (IPipelineTranslator를 상속받아야 함)
     * @param initializer ImportProfile에 기본 설정을 채우는 콜백
     */
    template <typename T, typename Fn>
        requires std::derived_from<T, IPipelineTranslator>
        && std::invocable<Fn, ImportProfile&>
    void RegisterPreset(Fn&& initializer);

    /**
     * Translator 타입에 맞는 기본 ImportProfile을 생성하여 반환합니다.
     * 등록된 프리셋이 없으면 빈 ImportProfile을 반환합니다.
     * @param translator_type Translator의 TypeId
     * @return 기본값이 채워진 ImportProfile
     */
    [[nodiscard]] ImportProfile GetDefaultProfile(const TypeId& translator_type) const;

    /**
     * 주어진 Translator TypeId에 대한 기본 프리셋 등록 여부를 확인합니다.
     * @param translator_type Translator의 TypeId
     * @return 해당 TypeId에 대한 프리셋이 등록되어 있으면 true, 아니면 false
     */
    [[nodiscard]] bool HasPreset(const TypeId& translator_type) const;

private:
    HashMap<TypeId, Function<void(ImportProfile&)>> preset_map;
};

template <typename T, typename Fn>
    requires std::derived_from<T, IPipelineTranslator>
    && std::invocable<Fn, ImportProfile&>
void ImportPresetManager::RegisterPreset(Fn&& initializer)
{
    RegisterPreset(TypeId::Of<T>(), std::forward<Fn>(initializer));
}
} // namespace se::editor
