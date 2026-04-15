#pragma once

#include "SimpleEditor/EditorCommon.h"


namespace se::editor
{
/**
 * GPU pick buffer의 clear 값. 이 값이 readback되면 커서 아래에 Entity가 없음을 의미합니다.
 * R32_UINT 텍스처를 float 0.0f로 clear하면 비트 패턴 0x00000000 = uint 0이 됩니다.
 */
constexpr uint32 ENTITY_PICK_MISS = 0;

/**
 * Entity ID를 GPU pick buffer에 기록할 값으로 인코딩합니다.
 * 0은 ENTITY_PICK_MISS(빈 공간)로 예약되어 있으므로, entity_id에 +1을 더해
 * 실제 entity id 0과 빈 공간을 구분합니다.
 */
constexpr uint32 EncodeEntityPickId(uint32 entity_id)
{
    return entity_id + 1;
}

/**
 * GPU pick buffer에서 readback한 값을 원래 Entity ID로 복원합니다.
 * 호출 전에 pick_id != ENTITY_PICK_MISS 검사가 필요합니다.
 */
constexpr uint32 DecodeEntityPickId(uint32 pick_id)
{
    return pick_id - 1;
}
} // namespace se::editor
