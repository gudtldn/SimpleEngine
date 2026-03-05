#pragma once

#include "SimpleEngine/Asset/Types/AssetBase.h"

#include <memory>


namespace se::asset
{
/**
 * 역직렬화된 에셋 데이터와 소멸자를 함께 보관하는 구조체
 *
 * shared_ptr의 커스텀 딜리터에 의존하던 소유권 모델을 분리하여,
 * 소멸자(destructor)를 SlotEntry에 직접 저장할 수 있도록 합니다.
 *
 * @note 소유권 이전(move) 후에는 ptr이 nullptr이 됩니다.
 */
struct AssetPayload
{
    AssetBase* ptr = nullptr;
    void(*destructor)(void*) = nullptr;

    /** Payload가 유효한 에셋 포인터를 보유하고 있는지 확인합니다. */
    [[nodiscard]] bool IsValid() const { return ptr != nullptr; }

    /** 명시적 bool 변환 연산자입니다. IsValid()와 동일합니다. */
    [[nodiscard]] explicit operator bool() const { return IsValid(); }
};
} // namespace se::asset
