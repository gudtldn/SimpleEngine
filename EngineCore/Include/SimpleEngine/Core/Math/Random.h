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
    /**
     * std::random_device를 사용하여 고유한 시드로 자동 초기화합니다.
     * @warning 매 프레임 생성하지 말고 클래스 멤버로 관리할 것을 권장합니다.
     */
    RandomStream();

    /** 특정 상태와 시퀀스로 스트림을 생성합니다. */
    RandomStream(u64 in_state, u64 in_seq = 0);

    /** 새로운 상태와 시퀀스로 시드를 설정합니다. */
    void Seed(u64 in_state, u64 in_seq = 0);

    /** 0 ~ 2^32-1 범위의 난수를 반환합니다. */
    u32 Next();

    /** [0, in_max) 범위의 정수 난수를 반환합니다. */
    u32 Range(u32 in_max);

    /** [in_min, in_max] 범위의 정수 난수를 반환합니다. */
    i32 Range(i32 in_min, i32 in_max);

    /** [0.0f, 1.0f) 범위의 실수 난수를 반환합니다. */
    f32 Float();

    /** [in_min, in_max) 범위의 실수 난수를 반환합니다. */
    f32 Range(f32 in_min, f32 in_max);

    /** 무작위로 true 또는 false를 반환합니다. */
    bool Bool();

private:
    u64 pcg_state; // PCG 내부 상태
    u64 pcg_inc;   // 스트림 증분 값
};

/**
 * 전역에서 사용할 수 있는 난수 유틸리티 구조체
 * @details 스레드 로컬 RandomStream을 사용하여 스레드 안전을 보장합니다.
 */
struct SE_CORE_API Random
{
    Random() = delete;

    /** 전역 난수 스트림의 시드를 설정합니다. */
    static void Seed(u64 in_state, u64 in_seq = 0);

    /** 0 ~ 2^32-1 범위의 난수를 반환합니다. */
    static u32 Next();

    /** [0, in_max) 범위의 정수 난수를 반환합니다. */
    static u32 Range(u32 in_max);

    /** [in_min, in_max] 범위의 정수 난수를 반환합니다. */
    static i32 Range(i32 in_min, i32 in_max);

    /** [0.0f, 1.0f) 범위의 실수 난수를 반환합니다. */
    static f32 Float();

    /** [in_min, in_max) 범위의 실수 난수를 반환합니다. */
    static f32 Range(f32 in_min, f32 in_max);

    /** 무작위로 true 또는 false를 반환합니다. */
    static bool Bool();
};
} // namespace se::math
