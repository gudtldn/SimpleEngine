#pragma once

#include "SimpleEditor/EditorCommon.h"
#include "SimpleEditor/Asset/ImportProfile.h"

#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Functional/Function.h"
#include "SimpleEngine/Core/Reflection/TypeId.h"


namespace se::editor
{
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
     * @param translator_type Translator의 TypeId (예: TypeId::Get<AssimpTranslator>())
     * @param initializer ImportProfile에 기본 설정을 채우는 콜백
     */
    void RegisterPreset(const TypeId& translator_type, Function<void(ImportProfile&)> initializer);

    /**
     * Translator 타입에 맞는 기본 ImportProfile을 생성하여 반환합니다.
     * 등록된 프리셋이 없으면 빈 ImportProfile을 반환합니다.
     * @param translator_type Translator의 TypeId
     * @return 기본값이 채워진 ImportProfile
     */
    [[nodiscard]] ImportProfile GetDefaultProfile(const TypeId& translator_type) const;

    /**
     * 소스 파일 확장자로부터 적절한 Translator TypeId를 찾아 기본 프로파일을 반환합니다.
     * 이 메서드는 등록된 모든 프리셋을 순회하지 않고, 호출자가 Translator TypeId를 이미 알고 있는 경우에 사용합니다.
     */
    [[nodiscard]] bool HasPreset(const TypeId& translator_type) const;

private:
    HashMap<TypeId, Function<void(ImportProfile&)>> preset_map;
};
} // namespace se::editor
