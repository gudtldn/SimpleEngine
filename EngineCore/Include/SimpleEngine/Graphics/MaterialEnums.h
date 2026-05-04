#pragma once

#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Core/Types/BitFlags.h"


namespace se
{
/** 머티리얼의 투명도 처리 방식 정의 */
enum class EBlendMode : uint8
{
    Opaque,      // 불투명 (Z버퍼에 기록, Alpha 무시)
    Masked,      // 알파 컷오프 (특정 알파 값 이하 픽셀 버림)
    Translucent, // 반투명 (Z버퍼 기록 안 함, Alpha 블렌딩)
    Additive,    // 가산 혼합 (빛/파티클 효과)
    Modulate,    // 곱셈 혼합
};

/** 셰이딩 방정식 정의 */
enum class EShadingModel : uint8
{
    Lit,        // 기본 PBR 셰이딩 (또는 Gouraud/Phong)
    Unlit,      // 빛의 영향을 받지 않음 (UI, 순수 색상)
    Subsurface, // 클리어코트 (자동차 도장 등)
    ClearCoat,  // 표면하 산란 (피부, 왁스 등)
};

/** 머티리얼의 렌더링 상태 비트 플래그 */
enum class EMaterialFlag : uint32
{
    None      = 0,
    AlphaTest = 1 << 0, // MASK 블렌드 모드용 알파 테스트 활성화
};
SE_ENABLE_BITMASK_OPERATORS(EMaterialFlag)
using MaterialFlags = BitFlags<EMaterialFlag>;
} // namespace se
