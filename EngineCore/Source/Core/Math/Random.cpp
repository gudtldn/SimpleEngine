#include "SimpleEngine/Core/Math/Random.h"

#include <bit>
#include <random>


namespace se::math
{
namespace
{
// PCG32 Constexpr Magic Numbers
// constexpr u64 PCG_DEFAULT_STATE = 0x853c49e6748fea9bULL;
// constexpr u64 PCG_DEFAULT_INC = 0xda3e39cb94b95bdbULL;
constexpr u64 PCG_MULTIPLIER = 6364136223846793005ULL;

constexpr u32 PCG_SHIFT_1 = 18U;
constexpr u32 PCG_SHIFT_2 = 27U;
constexpr u32 PCG_ROT_SHIFT = 59U;

constexpr u32 FLOAT_MASK_SHIFT = 8U;
constexpr f32 FLOAT_NORMALIZER = 1.0f / 16777216.0f; // 1.0f / (1 << 24)

thread_local RandomStream tl_random_stream;
} // namespace

RandomStream::RandomStream()
{
    std::random_device rd;
    const u64 seed_state = (static_cast<u64>(rd()) << 32) | rd();
    const u64 seed_seq = (static_cast<u64>(rd()) << 32) | rd();
    Seed(seed_state, seed_seq);
}

RandomStream::RandomStream(u64 in_state, u64 in_seq)
{
    Seed(in_state, in_seq);
}

void RandomStream::Seed(u64 in_state, u64 in_seq)
{
    pcg_state = 0ULL;
    pcg_inc = (in_seq << 1ULL) | 1ULL; // 스트림 ID는 항상 홀수여야 하므로 최하위 비트를 1로 설정
    Next();
    pcg_state += in_state;
    Next();
}

u32 RandomStream::Next()
{
    const u64 old_state = pcg_state;

    // LCG step
    pcg_state = (old_state * PCG_MULTIPLIER) + pcg_inc;

    // Calculate output function (XSH-RR)
    const u32 xorshifted = static_cast<u32>(((old_state >> PCG_SHIFT_1) ^ old_state) >> PCG_SHIFT_2);
    const int rot = static_cast<int>(old_state >> PCG_ROT_SHIFT);

    // Bit rotation
    return std::rotr(xorshifted, rot);
}

u32 RandomStream::Range(u32 in_max)
{
    if (in_max == 0)
    {
        return 0;
    }

    // --- Modulo Bias 제거 (Lemire's Fast Range Algorithm) ---

    // 1. Fast Path: 느린 % 연산 대신 64비트 곱셈 사용
    // (상위 32비트 = 결과값 / 하위 32비트 = 찌꺼기)
    u64 multi_result = static_cast<u64>(Next()) * in_max;
    u32 leftover = static_cast<u32>(multi_result);

    // 2. 편향 검사: 찌꺼기가 in_max보다 작을 때만 진입 (극히 희박한 확률)
    if (leftover < in_max)
    {
        // 3. Slow Path: 여기서만 무거운 % 연산을 1회 수행하여 편향 경계값 계산
        const u32 threshold = (~in_max + 1U) % in_max;

        // 편향 구간을 벗어날 때까지 다시 뽑기 (Rejection)
        while (leftover < threshold)
        {
            multi_result = static_cast<u64>(Next()) * in_max;
            leftover = static_cast<u32>(multi_result);
        }
    }

    // 4. 결과 반환: 범위가 맞춰진 상위 32비트만 추출
    return static_cast<u32>(multi_result >> 32);
}

i32 RandomStream::Range(i32 in_min, i32 in_max)
{
    if (in_min >= in_max)
    {
        return in_min;
    }

    // Integer Overflow 방지
    const u64 diff = static_cast<u64>(in_max) - static_cast<u64>(in_min);
    const u32 range_len = static_cast<u32>(diff) + 1;

    return in_min + static_cast<i32>(Range(range_len));
}

f32 RandomStream::Float()
{
    // 상위 24비트만 사용하여 f32 정밀도에 맞게 나누어 반환 ([0, 1) 범위)
    return static_cast<f32>(Next() >> FLOAT_MASK_SHIFT) * FLOAT_NORMALIZER;
}

f32 RandomStream::Range(f32 in_min, f32 in_max)
{
    if (in_min >= in_max)
    {
        return in_min;
    }

    return in_min + (Float() * (in_max - in_min));
}

bool RandomStream::Bool()
{
    return (Next() & (1U << 31)) != 0U;
}

void Random::Seed(u64 in_state, u64 in_seq)
{
    tl_random_stream.Seed(in_state, in_seq);
}

u32 Random::Next()
{
    return tl_random_stream.Next();
}

u32 Random::Range(u32 in_max)
{
    return tl_random_stream.Range(in_max);
}

i32 Random::Range(i32 in_min, i32 in_max)
{
    return tl_random_stream.Range(in_min, in_max);
}

f32 Random::Float()
{
    return tl_random_stream.Float();
}

f32 Random::Range(f32 in_min, f32 in_max)
{
    return tl_random_stream.Range(in_min, in_max);
}

bool Random::Bool()
{
    return tl_random_stream.Bool();
}
} // namespace se::math
