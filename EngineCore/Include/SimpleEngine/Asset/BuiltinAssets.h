#pragma once

#include "SimpleEngine/Asset/AssetId.h"
#include "SimpleEngine/Core/Types/Guid.h"


namespace se
{
// forward declaration
class AssetSubsystem;

/**
 * 엔진이 항상 보유하는 빌트인 에셋의 well-known AssetId 상수들
 * EScopeLayer::Global로 등록되어 Eviction 대상에서 제외됩니다.
 */
struct BuiltinAssetIds
{
    BuiltinAssetIds() = delete;

    static constexpr AssetId DefaultLit = AssetId{ Guid::FromString("00000001-0000-0000-0000-000000000001") };
    static constexpr AssetId DefaultLitInstance = AssetId{ Guid::FromString("00000001-0000-0000-0000-000000000002") };
    static constexpr AssetId White1x1 = AssetId{ Guid::FromString("00000001-0000-0000-0000-000000000003") };
}; //

/**
 * DefaultLit, DefaultLitInstance, White1x1을 AssetSubsystem에 Global scope로 등록합니다.
 * White1x1의 GPU 업로드는 EnsureTexturesResident()에서 별도로 수행합니다.
 */
SE_CORE_API void SeedBuiltinAssets(AssetSubsystem& subsystem);
} // namespace se
