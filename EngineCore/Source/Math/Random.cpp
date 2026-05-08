#include "SimpleEngine/Math/Random.h"

#include <bit>
#include <random>


namespace se::math
{
namespace
{
// PCG32 Constexpr Magic Numbers
// constexpr uint64 PCG_DEFAULT_STATE = 0x853c49e6748fea9bULL;
// constexpr uint64 PCG_DEFAULT_INC = 0xda3e39cb94b95bdbULL;
constexpr uint64 PCG_MULTIPLIER = 6364136223846793005ULL;

constexpr uint32 PCG_SHIFT_1 = 18U;
constexpr uint32 PCG_SHIFT_2 = 27U;
constexpr uint32 PCG_ROT_SHIFT = 59U;

constexpr uint32 FLOAT_MASK_SHIFT = 8U;
constexpr float FLOAT_NORMALIZER = 1.0f / 16777216.0f; // 1.0f / (1 << 24)

thread_local RandomStream tl_random_stream;
} // namespace

RandomStream::RandomStream()
{
    std::random_device rd;
    const uint64 seed_state = (static_cast<uint64>(rd()) << 32) | rd();
    const uint64 seed_seq = (static_cast<uint64>(rd()) << 32) | rd();
    Seed(seed_state, seed_seq);
}

RandomStream::RandomStream(uint64 in_state, uint64 in_seq)
{
    Seed(in_state, in_seq);
}

void RandomStream::Seed(uint64 in_state, uint64 in_seq)
{
    pcg_state = 0ULL;
    pcg_inc = (in_seq << 1ULL) | 1ULL; // 스트림 ID는 항상 홀수여야 하므로 최하위 비트를 1로 설정
    Next();
    pcg_state += in_state;
    Next();
}

uint32 RandomStream::Next()
{
    const uint64 old_state = pcg_state;

    // LCG step
    pcg_state = (old_state * PCG_MULTIPLIER) + pcg_inc;

    // Calculate output function (XSH-RR)
    const uint32 xorshifted = static_cast<uint32>(((old_state >> PCG_SHIFT_1) ^ old_state) >> PCG_SHIFT_2);
    const int rot = static_cast<int>(old_state >> PCG_ROT_SHIFT);

    // Bit rotation
    return std::rotr(xorshifted, rot);
}

uint32 RandomStream::Range(uint32 in_max)
{
    if (in_max == 0)
    {
        return 0;
    }

    // --- Modulo Bias 제거 (Lemire's Fast Range Algorithm) ---

    // 1. Fast Path: 느린 % 연산 대신 64비트 곱셈 사용
    // (상위 32비트 = 결과값 / 하위 32비트 = 찌꺼기)
    uint64 multi_result = static_cast<uint64>(Next()) * in_max;
    uint32 leftover = static_cast<uint32>(multi_result);

    // 2. 편향 검사: 찌꺼기가 in_max보다 작을 때만 진입 (극히 희박한 확률)
    if (leftover < in_max)
    {
        // 3. Slow Path: 여기서만 무거운 % 연산을 1회 수행하여 편향 경계값 계산
        const uint32 threshold = (~in_max + 1U) % in_max;

        // 편향 구간을 벗어날 때까지 다시 뽑기 (Rejection)
        while (leftover < threshold)
        {
            multi_result = static_cast<uint64>(Next()) * in_max;
            leftover = static_cast<uint32>(multi_result);
        }
    }

    // 4. 결과 반환: 범위가 맞춰진 상위 32비트만 추출
    return static_cast<uint32>(multi_result >> 32);
}

int32 RandomStream::Range(int32 in_min, int32 in_max)
{
    if (in_min >= in_max)
    {
        return in_min;
    }

    // Integer Overflow 방지
    const uint64 diff = static_cast<uint64>(in_max) - static_cast<uint64>(in_min);
    const uint32 range_len = static_cast<uint32>(diff) + 1;

    return in_min + static_cast<int32>(Range(range_len));
}

float RandomStream::Float()
{
    // 상위 24비트만 사용하여 float 정밀도에 맞게 나누어 반환 ([0, 1) 범위)
    return static_cast<float>(Next() >> FLOAT_MASK_SHIFT) * FLOAT_NORMALIZER;
}

float RandomStream::Range(float in_min, float in_max)
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

void Random::Seed(uint64 in_state, uint64 in_seq)
{
    tl_random_stream.Seed(in_state, in_seq);
}

uint32 Random::Next()
{
    return tl_random_stream.Next();
}

uint32 Random::Range(uint32 in_max)
{
    return tl_random_stream.Range(in_max);
}

int32 Random::Range(int32 in_min, int32 in_max)
{
    return tl_random_stream.Range(in_min, in_max);
}

float Random::Float()
{
    return tl_random_stream.Float();
}

float Random::Range(float in_min, float in_max)
{
    return tl_random_stream.Range(in_min, in_max);
}

bool Random::Bool()
{
    return tl_random_stream.Bool();
}
} // namespace se::math
