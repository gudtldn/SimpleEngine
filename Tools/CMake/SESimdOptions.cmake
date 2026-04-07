# =============================================================================
# Tools/CMake/SESimdOptions.cmake
# SE_DefaultOptions INTERFACE 타겟을 위한 SIMD 명령어 세트 설정
#
# 계층 구조 (상위 레벨은 하위 레벨을 누적 포함):
#   X86: NONE < SSE2 < SSE4_1 < AVX < AVX2 (AVX2는 FMA를 포함함)
#   ARM: NONE < NEON (AArch64 아키텍처에서는 NEON이 기본 지원됨)
#
# =============================================================================

# -- Architecture detection ---------------------------------------------------
if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64|i[3-6]86")
    set(_SE_ARCH_X86 TRUE)
    set(_SE_ARCH_ARM FALSE)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64|ARM64")
    set(_SE_ARCH_X86 FALSE)
    set(_SE_ARCH_ARM TRUE)
else()
    set(_SE_ARCH_X86 FALSE)
    set(_SE_ARCH_ARM FALSE)
endif()

# -- SIMD level option --------------------------------------------------------
if(_SE_ARCH_X86)
    set(SE_SIMD_LEVEL "AVX2" CACHE STRING "Minimum SIMD instruction set (x86)")
    set_property(CACHE SE_SIMD_LEVEL PROPERTY STRINGS "NONE" "SSE2" "SSE4_1" "AVX" "AVX2")
elseif(_SE_ARCH_ARM)
    set(SE_SIMD_LEVEL "NEON" CACHE STRING "SIMD instruction set (ARM)")
    set_property(CACHE SE_SIMD_LEVEL PROPERTY STRINGS "NONE" "NEON")
else()
    set(SE_SIMD_LEVEL "NONE" CACHE STRING "SIMD instruction set")
endif()

message(STATUS "SE SIMD level: ${SE_SIMD_LEVEL} (${CMAKE_SYSTEM_PROCESSOR})")

# -- Numeric level for cumulative comparison ----------------------------------
set(_SIMD_NUM_NONE   0)
set(_SIMD_NUM_SSE2   1)
set(_SIMD_NUM_SSE4_1 2)
set(_SIMD_NUM_AVX    3)
set(_SIMD_NUM_AVX2   4)
set(_SIMD_NUM_NEON   1)

if(DEFINED _SIMD_NUM_${SE_SIMD_LEVEL})
    set(_SIMD_CURRENT_NUM ${_SIMD_NUM_${SE_SIMD_LEVEL}})
else()
    message(WARNING "Unknown SE_SIMD_LEVEL: ${SE_SIMD_LEVEL}, defaulting to NONE")
    set(_SIMD_CURRENT_NUM 0)
endif()

# -- Apply compile definitions ------------------------------------------------
set(_SE_SIMD_DEFS)

if(_SE_ARCH_X86 AND _SIMD_CURRENT_NUM GREATER_EQUAL ${_SIMD_NUM_SSE2})
    list(APPEND _SE_SIMD_DEFS SE_CMAKE_SIMD_SSE2)
endif()
if(_SE_ARCH_X86 AND _SIMD_CURRENT_NUM GREATER_EQUAL ${_SIMD_NUM_SSE4_1})
    list(APPEND _SE_SIMD_DEFS SE_CMAKE_SIMD_SSE4_1)
endif()
if(_SE_ARCH_X86 AND _SIMD_CURRENT_NUM GREATER_EQUAL ${_SIMD_NUM_AVX})
    list(APPEND _SE_SIMD_DEFS SE_CMAKE_SIMD_AVX)
endif()
if(_SE_ARCH_X86 AND _SIMD_CURRENT_NUM GREATER_EQUAL ${_SIMD_NUM_AVX2})
    list(APPEND _SE_SIMD_DEFS SE_CMAKE_SIMD_AVX2 SE_CMAKE_SIMD_FMA)
endif()
if(_SE_ARCH_ARM AND _SIMD_CURRENT_NUM GREATER_EQUAL ${_SIMD_NUM_NEON})
    list(APPEND _SE_SIMD_DEFS SE_CMAKE_SIMD_NEON)
endif()

target_compile_definitions(SE_DefaultOptions INTERFACE ${_SE_SIMD_DEFS})

# -- Apply compiler flags -----------------------------------------------------
if(MSVC)
    # MSVC: /arch:SSE2가 기본값이며, /arch:AVX는 AVX 활성화, /arch:AVX2는 AVX2 및 FMA를 활성화함
    if(SE_SIMD_LEVEL STREQUAL "AVX")
        target_compile_options(SE_DefaultOptions INTERFACE /arch:AVX)
    elseif(SE_SIMD_LEVEL STREQUAL "AVX2")
        target_compile_options(SE_DefaultOptions INTERFACE /arch:AVX2)
    endif()
    # SSE2, SSE4_1: 별도의 플래그가 필요하지 않음 (SSE2는 기본값이며, SSE4.1은 독립된 플래그가 없음)

elseif(_SE_ARCH_X86)
    # GCC / Clang
    if(SE_SIMD_LEVEL STREQUAL "SSE2")
        target_compile_options(SE_DefaultOptions INTERFACE -msse2)
    elseif(SE_SIMD_LEVEL STREQUAL "SSE4_1")
        target_compile_options(SE_DefaultOptions INTERFACE -msse4.1)
    elseif(SE_SIMD_LEVEL STREQUAL "AVX")
        target_compile_options(SE_DefaultOptions INTERFACE -mavx)
    elseif(SE_SIMD_LEVEL STREQUAL "AVX2")
        target_compile_options(SE_DefaultOptions INTERFACE -mavx2 -mfma)
    endif()

elseif(_SE_ARCH_ARM)
    # AArch64: NEON이 항상 지원되므로 별도 플래그가 필요하지 않음
endif()
