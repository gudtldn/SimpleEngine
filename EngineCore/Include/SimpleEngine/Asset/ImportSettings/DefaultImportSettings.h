#pragma once
#include "SimpleEngine/Asset/ImportSettings/IAssetImportSettings.h"


namespace se::asset
{
/**
 * 별도 옵션이 필요 없는 에셋이나 폴백(Fallback)을 위한 기본 설정 클래스
 */
class DefaultImportSettings final : public IAssetImportSettings{};
}
