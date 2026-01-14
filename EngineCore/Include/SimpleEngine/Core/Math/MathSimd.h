#pragma once
#include "SimpleEngine/Core/HAL/CpuFeature.h"
#include "SimpleEngine/Core/Math/MathFwd.h"

// 플랫폼에 따른 인트린식 헤더 포함
#if SE_ARCH_X86_FAMILY
#include <immintrin.h>
#elif SE_ARCH_ARM_FAMILY
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

#if SE_ARCH_X86_FAMILY
/** float 행렬 곱셈을 SSE를 사용하여 구현 (외부 곱 방식) */
inline void Matrix4x4MultiplySSEImpl(const float* lhs, const float* rhs, float* result)
{
    // 1. rhs 행렬의 4개의 행을 각각 128비트 SSE 레지스터(__m128)로 로드
    // _mm_load_ps: 16바이트 정렬된 주소에서 4개의 float를 로드
    const __m128 rhs_row0 = _mm_load_ps(&rhs[0]);
    const __m128 rhs_row1 = _mm_load_ps(&rhs[4]);
    const __m128 rhs_row2 = _mm_load_ps(&rhs[8]);
    const __m128 rhs_row3 = _mm_load_ps(&rhs[12]);

    // 2. lhs의 각 행에 대해서 반복하여 result의 i번째 행을 계산
    for (uint32 i = 0; i < 4; ++i)
    {
        const float* current_lhs_row = &lhs[i * 4];

        // 3. lhs 행의 각 스칼라 값을 SSE 레지스터의 모든 요소로 복제(broadcast)
        // _mm_set1_ps(value): [value, value, value, value] 형태의 레지스터를 생성
        const __m128 lhs_broadcast0 = _mm_set1_ps(current_lhs_row[0]);
        const __m128 lhs_broadcast1 = _mm_set1_ps(current_lhs_row[1]);
        const __m128 lhs_broadcast2 = _mm_set1_ps(current_lhs_row[2]);
        const __m128 lhs_broadcast3 = _mm_set1_ps(current_lhs_row[3]);

        // 4. 외부 곱(_mm_mul_ps)과 덧셈(_mm_add_ps)을 순차적으로 수행
        const __m128 term0 = _mm_mul_ps(lhs_broadcast0, rhs_row0);
        const __m128 term1 = _mm_mul_ps(lhs_broadcast1, rhs_row1);
        const __m128 term2 = _mm_mul_ps(lhs_broadcast2, rhs_row2);
        const __m128 term3 = _mm_mul_ps(lhs_broadcast3, rhs_row3);

        const __m128 sum01 = _mm_add_ps(term0, term1);
        const __m128 sum23 = _mm_add_ps(term2, term3);
        const __m128 result_row = _mm_add_ps(sum01, sum23);

        // 5. 계산된 결과 행을 다시 메모리에 저장
        _mm_store_ps(&result[i * 4], result_row);
    }
}

/** float 행렬 곱셈을 FMA3 명령어를 사용하여 구현 (외부 곱 방식) */
inline void Matrix4x4MultiplyFMAImpl(const float* lhs, const float* rhs, float* result)
{
    // 1. rhs 행렬의 4개의 행을 미리 로드
    const __m128 rhs_row0 = _mm_load_ps(&rhs[0]);
    const __m128 rhs_row1 = _mm_load_ps(&rhs[4]);
    const __m128 rhs_row2 = _mm_load_ps(&rhs[8]);
    const __m128 rhs_row3 = _mm_load_ps(&rhs[12]);

    // 2. lhs의 각 행에 대해서 반복
    for (uint32 i = 0; i < 4; ++i)
    {
        const float* current_lhs_row = &lhs[i * 4];

        // 3. lhs 행의 각 스칼라 값을 복제(broadcast)
        const __m128 lhs_broadcast0 = _mm_set1_ps(current_lhs_row[0]);
        const __m128 lhs_broadcast1 = _mm_set1_ps(current_lhs_row[1]);
        const __m128 lhs_broadcast2 = _mm_set1_ps(current_lhs_row[2]);
        const __m128 lhs_broadcast3 = _mm_set1_ps(current_lhs_row[3]);

        // 4. FMA(Fused Multiply-Add)를 사용하여 외부 곱과 덧셈을 누적 계산
        // _mm_fmadd_ps(a, b, c) -> (a * b) + c

        // 첫 번째 외부 곱 계산
        __m128 result_row = _mm_mul_ps(lhs_broadcast0, rhs_row0);

        // 두 번째부터 FMA를 사용하여 누적
        result_row = _mm_fmadd_ps(lhs_broadcast1, rhs_row1, result_row);
        result_row = _mm_fmadd_ps(lhs_broadcast2, rhs_row2, result_row);
        result_row = _mm_fmadd_ps(lhs_broadcast3, rhs_row3, result_row);

        // 5. 계산된 결과 행을 메모리에 저장
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

#elif SE_ARCH_ARM_FAMILY

template <traits::FloatingType T>
void Matrix4x4MultiplyNEONImpl(const T* lhs, const T* rhs, T* result)
{
    // TODO: NEON 구현 추가
}
#endif
}

template <traits::FloatingType T>
Matrix4x4Impl<T> Matrix4x4Multiply(const Matrix4x4Impl<T>& lhs, const Matrix4x4Impl<T>& rhs)
{
    // 나중에 성능을 더 끌어올리고 싶으면 프로그램 시작때 CPU Feature를 검사한 다음, 적절한 함수 포인터를 설정하고 호출하는 방법이 있음
    Matrix4x4Impl<T> result;

    const T* lhs_ptr = reinterpret_cast<const T*>(&lhs);
    const T* rhs_ptr = reinterpret_cast<const T*>(&rhs);
    T* result_ptr = reinterpret_cast<T*>(&result);

    if constexpr (std::same_as<T, float>)
    {
#if SE_ARCH_X86_FAMILY
        if (core::CpuFeature::HasSSE4_1())
        {
            if (core::CpuFeature::HasFMA3())
            {
                details::Matrix4x4MultiplyFMAImpl(lhs_ptr, rhs_ptr, result_ptr);
            }
            else
            {
                details::Matrix4x4MultiplySSEImpl(lhs_ptr, rhs_ptr, result_ptr);
            }
            return result;
        }
        // TODO: 다른 SIMD 버전에 대해서 구현
#elif SE_ARCH_ARM_FAMILY
        if (core::CpuFeature::HasNEON())
        {
            // TODO: NEON 구현 추가
        }
#endif
    }
    else if constexpr (std::same_as<T, double>)
    {
#if SE_ARCH_X86_FAMILY
        if (core::CpuFeature::HasAVX() && core::CpuFeature::HasFMA3())
        {
            details::Matrix4x4MultiplyAVXImpl(lhs_ptr, rhs_ptr, result_ptr);
            return result;
        }
        // TODO: 다른 SIMD 버전에 대해서 구현
#endif
    }

    // SIMD 미지원 시 일반 구현 사용
    details::Matrix4x4MultiplyGeneric(lhs_ptr, rhs_ptr, result_ptr);
    return result;
}
}
