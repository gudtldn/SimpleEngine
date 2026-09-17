#pragma once

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Core/Reflection/TypeId.h"


namespace se
{
/** 실제 객체의 시작점 기준으로 계산한 부모 타입의 바이트 오프셋 */
struct CastEntry
{
    TypeId type;
    usize offset = 0;
};

/** 타입 하나의 캐스팅 테이블 */
struct TypeRecord
{
    TypeId id;
    Array<CastEntry> all_bases; // offset 0으로 포함
};
} // namespace se
