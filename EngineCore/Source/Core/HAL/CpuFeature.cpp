#include "SimpleEngine/Core/HAL/CpuFeature.h"

#if SE_ARCH_X86_FAMILY
#include "cpuinfo_x86.h"
#elif SE_ARCH_ARM64
#include "cpuinfo_arm.h"
#endif
using namespace cpu_features;


namespace
{
#if SE_ARCH_X86_FAMILY
X86Features GetFeatures()
{
    static const X86Info info = GetX86Info();
    return info.features;
}
#elif SE_ARCH_ARM64
ArmFeatures GetFeatures()
{
    static const ArmInfo info = GetArmInfo();
    return info.features;
}
#endif
} // namespace

namespace se
{
#if SE_ARCH_X86_FAMILY
bool CpuFeature::HasSSE()     { return CPU_FEATURES_COMPILED_X86_SSE || GetFeatures().sse;       }
bool CpuFeature::HasSSE2()    { return CPU_FEATURES_COMPILED_X86_SSE2 || GetFeatures().sse2;     }
bool CpuFeature::HasSSE3()    { return CPU_FEATURES_COMPILED_X86_SSE3 || GetFeatures().sse3;     }
bool CpuFeature::HasSSSE3()   { return CPU_FEATURES_COMPILED_X86_SSSE3 || GetFeatures().ssse3;   }
bool CpuFeature::HasSSE4_1()  { return CPU_FEATURES_COMPILED_X86_SSE4_1 || GetFeatures().sse4_1; }
bool CpuFeature::HasSSE4_2()  { return CPU_FEATURES_COMPILED_X86_SSE4_2 || GetFeatures().sse4_2; }
bool CpuFeature::HasAVX()     { return CPU_FEATURES_COMPILED_X86_AVX || GetFeatures().avx;       }
bool CpuFeature::HasAVX2()    { return CPU_FEATURES_COMPILED_X86_AVX2 || GetFeatures().avx2;     }
bool CpuFeature::HasFMA3()    { return GetFeatures().fma3;                                       }
bool CpuFeature::HasFMA4()    { return GetFeatures().fma4;                                       }
bool CpuFeature::HasAVX512F() { return GetFeatures().avx512f;                                    }
bool CpuFeature::HasNEON()    { return false;                                                    }

#elif SE_ARCH_ARM64
bool CpuFeature::HasSSE()     { return false;                                                    }
bool CpuFeature::HasSSE2()    { return false;                                                    }
bool CpuFeature::HasSSE3()    { return false;                                                    }
bool CpuFeature::HasSSSE3()   { return false;                                                    }
bool CpuFeature::HasSSE4_1()  { return false;                                                    }
bool CpuFeature::HasSSE4_2()  { return false;                                                    }
bool CpuFeature::HasAVX()     { return false;                                                    }
bool CpuFeature::HasAVX2()    { return false;                                                    }
bool CpuFeature::HasFMA3()    { return false;                                                    }
bool CpuFeature::HasFMA4()    { return false;                                                    }
bool CpuFeature::HasAVX512F() { return false;                                                    }
bool CpuFeature::HasNEON()    { return CPU_FEATURES_COMPILED_ANY_ARM_NEON || GetFeatures().neon; }
#endif
} // namespace se
