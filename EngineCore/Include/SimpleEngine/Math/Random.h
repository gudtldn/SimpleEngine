#pragma once

#include "SimpleEngine/Core/HAL/PlatformTypes.h"


namespace se::math
{
/**
 * PCG32 기반의 고성능 난수 스트림 클래스
 */
class SE_CORE_API RandomStream
{
public:
    RandomStream();
    RandomStream(uint64 in_state, uint64 in_seq = 0);

    /** 새로운 상태와 시퀀스로 시드를 설정합니다. */
    void Seed(uint64 in_state, uint64 in_seq = 0);

    /** 0 ~ 2^32-1 범위의 난수를 반환합니다. */
    uint32 Next();

    /** [0, in_max) 범위의 정수 난수를 반환합니다. */
    uint32 Range(uint32 in_max);

    /** [in_min, in_max] 범위의 정수 난수를 반환합니다. */
    int32 Range(int32 in_min, int32 in_max);

    /** [0.0f, 1.0f) 범위의 실수 난수를 반환합니다. */
    float Float();

    /** [in_min, in_max) 범위의 실수 난수를 반환합니다. */
    float Range(float in_min, float in_max);

    /** 무작위로 true 또는 false를 반환합니다. */
    bool Bool();

private:
    uint64 pcg_state; // PCG 내부 상태
    uint64 pcg_inc;   // 스트림 증분 값
};

/**
 * 전역에서 사용할 수 있는 난수 유틸리티 구조체
 * @details 스레드 로컬 RandomStream을 사용하여 스레드 안전을 보장합니다.
 */
struct SE_CORE_API Random
{
    Random() = delete;

    /** 전역 난수 스트림의 시드를 설정합니다. */
    static void Seed(uint64 in_state, uint64 in_seq = 0);

    /** 0 ~ 2^32-1 범위의 난수를 반환합니다. */
    static uint32 Next();

    /** [0, in_max) 범위의 정수 난수를 반환합니다. */
    static uint32 Range(uint32 in_max);

    /** [in_min, in_max] 범위의 정수 난수를 반환합니다. */
    static int32 Range(int32 in_min, int32 in_max);

    /** [0.0f, 1.0f) 범위의 실수 난수를 반환합니다. */
    static float Float();

    /** [in_min, in_max) 범위의 실수 난수를 반환합니다. */
    static float Range(float in_min, float in_max);

    /** 무작위로 true 또는 false를 반환합니다. */
    static bool Bool();
};
} // namespace se::math
