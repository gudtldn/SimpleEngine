#pragma once

#include "SimpleEngine/Core/Container/Optional.h"


namespace se::editor
{
/**
 * GPU Color ID Picking의 인코딩/디코딩을 캡슐화하는 값 타입
 *
 * GPU 규약:
 * - R32_UINT 텍스처를 float 0.0f로 clear -> 비트 패턴 0x00000000 = uint 0
 * - 0 = "빈 공간" (Entity 없음)
 * - entity_id + 1 = 유효한 Entity가 기록된 값
 *
 * CPU 측에서 Decode()를 통해 Optional<uint32>로 변환하여,
 * miss/hit을 타입 안전하게 구분합니다.
 */
struct EntityPickId
{
    /** GPU clear 값 = 빈 공간. R32_UINT 0.0f clear -> 0x00000000 */
    static constexpr uint32 ENCODED_NONE = 0;

public:
    /** 빈 공간(miss)을 나타내는 결과 생성 */
    static constexpr EntityPickId None() { return { ENCODED_NONE }; }

    /** Entity ID를 인코딩하여 GPU 기록용 결과 생성 (entity_id + 1) */
    static constexpr EntityPickId Encode(uint32 entity_id) { return { entity_id + 1 }; }

    /** GPU readback 원시 값으로부터 결과 생성 */
    static constexpr EntityPickId FromRaw(uint32 raw) { return { raw }; }

public:
    /** 유효한 Entity가 pick되었는지 여부 */
    [[nodiscard]] constexpr bool HasEntity() const { return encoded != ENCODED_NONE; }

    /** 디코딩된 entity id 반환. miss이면 NullOpt */
    [[nodiscard]] constexpr Optional<uint32> Decode() const
    {
        if (encoded == ENCODED_NONE)
        {
            return NullOpt;
        }
        return encoded - 1;
    }

public:
    /** GPU에서 읽어온 인코딩된 값 (0 = miss, entity_id + 1 = hit) */
    uint32 encoded = ENCODED_NONE;
};
} // namespace se::editor
