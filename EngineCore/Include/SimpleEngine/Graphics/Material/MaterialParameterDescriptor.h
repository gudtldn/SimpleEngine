#pragma once

#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Core/Math/Math.h"
#include "SimpleEngine/Core/Reflection/Annotations.h"
#include "SimpleEngine/Core/Types/StringName.h"


namespace se
{
/** 머티리얼 UBO 파라미터 타입 (HLSL 대응) */
enum class EMaterialParamType : uint8
{
    Float,  // 4 bytes
    Float2, // 8 bytes
    Float3, // 12 bytes
    Float4, // 16 bytes
};

/**
 * 머티리얼 파라미터 레이아웃 정보
 */
struct SE_ANNOTATION(=meta::Reflect) MaterialParameterDescriptor
{
    // 파라미터 식별 이름 (예: "BaseColor")
    SE_ANNOTATION(=meta::Property)
    StringName name;

    // 데이터 타입
    SE_ANNOTATION(=meta::Property)
    EMaterialParamType type = EMaterialParamType::Float4;

    // 버퍼 내 바이트 오프셋
    SE_ANNOTATION(=meta::Property)
    uint32 offset = 0;

    // 인스턴스 생성 시 기본값
    SE_ANNOTATION(=meta::Property)
    Vector4f default_value = {};

    /** 이 파라미터가 차지하는 바이트 크기를 반환합니다. */
    [[nodiscard]] uint32 GetSize() const;

    /** 셰이더 std140/std430 레이아웃에 따른 정렬(Alignment) 요구사항을 반환합니다. */
    [[nodiscard]] uint32 GetAlignment() const;
};
} // namespace se

SE_DECLARE_REFLECTION(se::MaterialParameterDescriptor)
