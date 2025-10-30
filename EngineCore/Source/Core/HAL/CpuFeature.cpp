#include "SimpleEngine/Core/HAL/CpuFeature.h"

#if SE_COMPILE_PLATFORM_X86_FAMILY
#include "cpuinfo_x86.h"
#elif SE_COMPILE_PLATFORM_ARM64
#include "cpuinfo_arm.h"
#endif

using namespace cpu_features;


namespace
{
#if SE_COMPILE_PLATFORM_X86_FAMILY
const X86Features Features = GetX86Info().features;
#elif SE_COMPILE_PLATFORM_ARM64
const ArmFeatures Features = GetArmInfo().features;
#endif
}

namespace se::core
{
#if SE_COMPILE_PLATFORM_X86_FAMILY
bool CpuFeature::HasSSE()     { return CPU_FEATURES_COMPILED_X86_SSE || Features.sse;       }
bool CpuFeature::HasSSE2()    { return CPU_FEATURES_COMPILED_X86_SSE2 || Features.sse2;     }
bool CpuFeature::HasSSE3()    { return CPU_FEATURES_COMPILED_X86_SSE3 || Features.sse3;     }
bool CpuFeature::HasSSSE3()   { return CPU_FEATURES_COMPILED_X86_SSSE3 || Features.ssse3;   }
bool CpuFeature::HasSSE4_1()  { return CPU_FEATURES_COMPILED_X86_SSE4_1 || Features.sse4_1; }
bool CpuFeature::HasSSE4_2()  { return CPU_FEATURES_COMPILED_X86_SSE4_2 || Features.sse4_2; }
bool CpuFeature::HasAVX()     { return CPU_FEATURES_COMPILED_X86_AVX || Features.avx;       }
bool CpuFeature::HasAVX2()    { return CPU_FEATURES_COMPILED_X86_AVX2 || Features.avx2;     }
bool CpuFeature::HasFMA3()    { return Features.fma3;                                       }
bool CpuFeature::HasFMA4()    { return Features.fma4;                                       }
bool CpuFeature::HasAVX512F() { return Features.avx512f;                                    }
bool CpuFeature::HasNEON()    { return false;                                               }

#elif SE_COMPILE_PLATFORM_ARM64

bool CpuFeature::HasSSE()     { return false;                                               }
bool CpuFeature::HasSSE2()    { return false;                                               }
bool CpuFeature::HasSSE3()    { return false;                                               }
bool CpuFeature::HasSSSE3()   { return false;                                               }
bool CpuFeature::HasSSE4_1()  { return false;                                               }
bool CpuFeature::HasSSE4_2()  { return false;                                               }
bool CpuFeature::HasAVX()     { return false;                                               }
bool CpuFeature::HasAVX2()    { return false;                                               }
bool CpuFeature::HasFMA3()    { return false;                                               }
bool CpuFeature::HasFMA4()    { return false;                                               }
bool CpuFeature::HasAVX512F() { return false;                                               }
bool CpuFeature::HasNEON()    { return CPU_FEATURES_COMPILED_ANY_ARM_NEON || Features.neon; }
#endif
}
