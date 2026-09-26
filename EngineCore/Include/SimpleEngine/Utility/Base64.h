#pragma once

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/ArrayView.h"
#include "SimpleEngine/Core/Container/Optional.h"
#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/Container/StringView.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"


namespace se
{
/**
 * base64 인코딩과 디코딩 유틸리티
 * RFC 4648의 표준 알파벳(A-Z, a-z, 0-9, +, /)과 '=' 패딩을 씁니다.
 */
namespace base64
{
/** bytes를 base64 문자열로 인코딩합니다. 예: "Man"은 "TWFu", "Ma"는 "TWE=" */
[[nodiscard]] SE_CORE_API String Encode(ArrayView<const u8> bytes);

/**
 * base64 문자열을 바이트로 디코딩합니다.
 * 길이가 4의 배수가 아니거나, 표준 알파벳이 아닌 문자, 끝이 아닌 곳의 패딩, 0이 아닌 남는 비트가 있으면 NullOpt를 돌려줍니다.
 */
[[nodiscard]] SE_CORE_API Optional<Array<u8>> Decode(StringView text);
} // namespace base64
} // namespace se
