#pragma once

#include <version>


#if defined(__cpp_impl_reflection) && defined(__cpp_lib_reflection)
#   define SE_HAS_REFLECTION true
#else
#   define SE_HAS_REFLECTION false
#endif

#if SE_HAS_REFLECTION
// C++26 리플렉션 사용 가능해지면, SE_HAS_REFLECTION가 있는곳을 모두 정리하기
#    error "C++26 reflection detected."
#endif
