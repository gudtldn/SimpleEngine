#pragma once
#include "SimpleEngine/Core/HAL/CpuFeature.h"
#include "SimpleEngine/Core/Math/MathFwd.h"

// 플랫폼에 따른 인트린식 헤더 포함
#if SE_PLATFORM_ARCHITECTURE_X86_FAMILY
#include <immintrin.h>
#elif SE_PLATFORM_ARCHITECTURE_ARM_FAMILY
#include <arm_neon.h>
#endif


namespace se::math::simd
{
namespace details
{
/** SIMD를 사용할 수 없을 때를 위한 Matrix4x4 곱셈 대체 구현 */
template <traits::FloatingType T>
void Matrix4x4MultiplyGeneric(const T* lhs, const T* rhs, T* result)
{
    for (uint32 i = 0; i < 4; ++i)
    {
        for (uint32 j = 0; j < 4; ++j)
        {
            result[i * 4 + j] = lhs[i * 4 + 0] * rhs[0 * 4 + j]
                              + lhs[i * 4 + 1] * rhs[1 * 4 + j]
                              + lhs[i * 4 + 2] * rhs[2 * 4 + j]
                              + lhs[i * 4 + 3] * rhs[3 * 4 + j];
        }
    }
}

#if SE_PLATFORM_ARCHITECTURE_X86_FAMILY
/** float 행렬 곱셈을 SSE를 사용하여 구현 (내적 방식) */
inline void Matrix4x4MultiplySSEImpl(const float* lhs, const float* rhs, float* result)
{
    // 1. rhs 행렬의 4개의 행을 각각 128비트 SSE 레지스터(__m128)로 로드 (행으로 로드하지만, 밑에서 전치를 할 예정이기 때문에 변수명을 col로 함)
    // _mm_load_ps: 16바이트 정렬된 주소에서 4개의 float를 로드
    __m128 rhs_col0 = _mm_load_ps(&rhs[0]);
    __m128 rhs_col1 = _mm_load_ps(&rhs[4]);
    __m128 rhs_col2 = _mm_load_ps(&rhs[8]);
    __m128 rhs_col3 = _mm_load_ps(&rhs[12]);

    // 2. 4x4 행렬을 그 자리에서 전치(transpose)
    _MM_TRANSPOSE4_PS(rhs_col0, rhs_col1, rhs_col2, rhs_col3);

    // 3. lhs의 각 행에 대해서 반복
    for (uint32 i = 0; i < 4; ++i)
    {
        // lhs의 현재 행을 레지스터로 로드
        __m128 lhs_row = _mm_load_ps(&lhs[i * 4]);

        // 4. 내적(Dot Product) 계산 (SSE4.1 명령어 사용)
        // _mm_dp_ps(a, b, mask): a와 b의 내적을 계산

        // 0xF1의 의미
        // - 상위 4비트(0xF = 1111): a와 b의 어떤 요소를 곱할지 결정 (모두 곱함)
        // - 하위 4비트(0x1 = 0001): 결과를 목적지 레지스터의 어디에 저장할지 결정 (첫 번째 요소에 저장)
        __m128 res_row0 = _mm_dp_ps(lhs_row, rhs_col0, 0xF1); // 결과: [c0, 0, 0, 0]
        __m128 res_row1 = _mm_dp_ps(lhs_row, rhs_col1, 0xF1); // 결과: [c1, 0, 0, 0]
        __m128 res_row2 = _mm_dp_ps(lhs_row, rhs_col2, 0xF1); // 결과: [c2, 0, 0, 0]
        __m128 res_row3 = _mm_dp_ps(lhs_row, rhs_col3, 0xF1); // 결과: [c3, 0, 0, 0]

        // 5. 각 레지스터의 첫 번째 요소에 있는 결과들을 하나의 레지스터로 합침
        __m128 temp01 = _mm_unpacklo_ps(res_row0, res_row1); // [c0, 0, 0, 0], [c1, 0, 0, 0] -> [c0, c1, 0, 0]
        __m128 temp23 = _mm_unpacklo_ps(res_row2, res_row3); // [c2, 0, 0, 0], [c3, 0, 0, 0] -> [c2, c3, 0, 0]
        __m128 result_row = _mm_movelh_ps(temp01, temp23);        // [c0, c1, 0, 0], [c2, c3, 0, 0] -> [c0, c1, c2, c3]

        // 6. 이렇게 계산된 행을 다시 메모리에 저장
        _mm_store_ps(&result[i * 4], result_row);
    }
}

/** double 행렬 곱셈을 AVX와 FMA를 사용하여 구현 (외부 곱 방식) */
inline void Matrix4x4MultiplyAVXImpl(const double* lhs, const double* rhs, double* result)
{
    // result의 각 행에 대해서 반복
    for (uint32 i = 0; i < 4; ++i)
    {
        const double* current_lhs_row = &lhs[i * 4];

        // 1. lhs 행의 첫 번째 스칼라 값(current_lhs_row[0])을 AVX 레지스터의 모든 4개 double 요소로 복제(broadcast)
        const __m256d lhs_broadcast0 = _mm256_broadcast_sd(&current_lhs_row[0]); // [s, s, s, s]

        // 1.1. rhs의 첫 번째 행 벡터를 로드
        const __m256d rhs_row0 = _mm256_load_pd(&rhs[0]);

        // 1.2. 첫 번째 외부 곱을 계산
        __m256d result_row = _mm256_mul_pd(lhs_broadcast0, rhs_row0); // result_row = lhs[i][0] * rhs_row_0

        // 2. 두 번째 요소부터는 FMA(Fused Multiply-Add)를 사용하여 곱셈과 덧셈을 한 번에 수행
        // _mm256_fmadd_pd(a, b, c)는 (a * b) + c 를 계산하는 함수
        const __m256d lhs_broadcast1 = _mm256_broadcast_sd(&current_lhs_row[1]);
        const __m256d rhs_row1 = _mm256_load_pd(&rhs[4]);
        result_row = _mm256_fmadd_pd(lhs_broadcast1, rhs_row1, result_row); // result_row = (lhs[i][1] * rhs_row_1) + result_row

        const __m256d lhs_broadcast2 = _mm256_broadcast_sd(&current_lhs_row[2]);
        const __m256d rhs_row2 = _mm256_load_pd(&rhs[8]);
        result_row = _mm256_fmadd_pd(lhs_broadcast2, rhs_row2, result_row); // result_row = (lhs[i][2] * rhs_row_2) + result_row

        const __m256d lhs_broadcast3 = _mm256_broadcast_sd(&current_lhs_row[3]);
        const __m256d rhs_row3 = _mm256_load_pd(&rhs[12]);
        result_row = _mm256_fmadd_pd(lhs_broadcast3, rhs_row3, result_row); // result_row = (lhs[i][3] * rhs_row_3) + result_row

        // 3. 이렇게 계산된 행을 다시 메모리에 저장
        _mm256_store_pd(&result[i * 4], result_row);
    }
}

#elif SE_PLATFORM_ARCHITECTURE_ARM_FAMILY

template <traits::FloatingType T>
void Matrix4x4MultiplyNEONImpl(const T* lhs, const T* rhs, T* result)
{
    // TODO: NEON 구현 추가
}
#endif
}

template <typename T>
    requires std::same_as<T, float>
Matrix4x4Impl<T> Matrix4x4Multiply(const Matrix4x4Impl<T>& lhs, const Matrix4x4Impl<T>& rhs)
{
    Matrix4x4Impl<T> result;

    const T* lhs_ptr = reinterpret_cast<const T*>(&lhs);
    const T* rhs_ptr = reinterpret_cast<const T*>(&rhs);
    T* result_ptr = reinterpret_cast<T*>(&result);

#if SE_PLATFORM_ARCHITECTURE_X86_FAMILY
    if (core::CpuFeature::HasSSE4_1())
    {
        details::Matrix4x4MultiplySSEImpl(lhs_ptr, rhs_ptr, result_ptr);
        return result;
    }
    // TODO: 다른 SIMD 버전에 대해서 구현
#elif SE_PLATFORM_ARCHITECTURE_ARM_FAMILY
    if (core::CpuFeature::HasNEON())
    {
        // TODO: NEON 구현 추가
    }
#endif

    // SIMD 미지원 시 일반 구현 사용
    details::Matrix4x4MultiplyGeneric(lhs_ptr, rhs_ptr, result_ptr);
    return result;
}

template <typename T>
    requires std::same_as<T, double>
Matrix4x4Impl<T> Matrix4x4Multiply(const Matrix4x4Impl<T>& lhs, const Matrix4x4Impl<T>& rhs)
{
    Matrix4x4Impl<T> result;

    const T* lhs_ptr = reinterpret_cast<const T*>(&lhs);
    const T* rhs_ptr = reinterpret_cast<const T*>(&rhs);
    T* result_ptr = reinterpret_cast<T*>(&result);

#if SE_PLATFORM_ARCHITECTURE_X86_FAMILY
    if (core::CpuFeature::HasAVX() && core::CpuFeature::HasFMA3())
    {
        details::Matrix4x4MultiplyAVXImpl(lhs_ptr, rhs_ptr, result_ptr);
        return result;
    }
    // TODO: 다른 SIMD 버전에 대해서 구현
#endif

    // SIMD 미지원 시 일반 구현 사용
    details::Matrix4x4MultiplyGeneric(lhs_ptr, rhs_ptr, result_ptr);
    return result;
}
}
