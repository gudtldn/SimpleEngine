#include "SimpleEngine/Utility/Base64.h"

#include <algorithm>


namespace se
{
namespace
{
/** 6비트 값 0~63에 대응하는 표준 알파벳 */
constexpr char ALPHABET[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/** 알파벳 문자 하나를 6비트 값으로 바꿉니다. 표준 알파벳이 아니면 NullOpt를 돌려줍니다. */
[[nodiscard]] Optional<u32> SextetOf(char c)
{
    if (c >= 'A' && c <= 'Z') { return static_cast<u32>(c - 'A'); }
    if (c >= 'a' && c <= 'z') { return static_cast<u32>(c - 'a' + 26); }
    if (c >= '0' && c <= '9') { return static_cast<u32>(c - '0' + 52); }
    if (c == '+')             { return 62u; }
    if (c == '/')             { return 63u; }
    return NullOpt;
}
} // namespace

namespace base64
{
String Encode(ArrayView<const u8> bytes)
{
    String text;
    text.Reserve(((bytes.Len() + 2) / 3) * 4);

    // 3바이트(24비트)를 6비트씩 네 글자로 바꿈. 마지막 묶음이 n바이트면 n + 1글자를 쓰고 나머지는 '='로 채움
    for (usize offset = 0; offset < bytes.Len(); offset += 3)
    {
        const usize byte_count = std::min<usize>(3, bytes.Len() - offset);
        u32 group = 0;
        for (usize i = 0; i < byte_count; ++i)
        {
            group |= static_cast<u32>(bytes[offset + i]) << (16 - (8 * i));
        }

        for (usize i = 0; i < 4; ++i)
        {
            text.Push(i <= byte_count ? ALPHABET[(group >> (18 - (6 * i))) & 0x3F] : '=');
        }
    }
    return text;
}

Optional<Array<u8>> Decode(StringView text)
{
    if (text.ByteLen() % 4 != 0)
    {
        return NullOpt;
    }

    Array<u8> bytes;
    bytes.Reserve((text.ByteLen() / 4) * 3);
    for (usize offset = 0; offset < text.ByteLen(); offset += 4)
    {
        // 패딩은 마지막 묶음의 끝 한두 글자에만 올 수 있음
        usize padding = 0;
        if (offset + 4 == text.ByteLen())
        {
            padding = text[offset + 3] != '=' ? 0 : (text[offset + 2] != '=' ? 1 : 2);
        }

        u32 group = 0;
        for (usize i = 0; i < 4 - padding; ++i)
        {
            const auto sextet = SextetOf(text[offset + i]);
            if (!sextet)
            {
                return NullOpt;
            }
            group |= *sextet << (18 - (6 * i));
        }

        // 바이트가 되지 못하고 남는 비트가 0이 아니면 같은 바이트를 다르게 쓴 문자열이므로 거부
        const u32 leftover_mask = (1u << (8 * padding)) - 1;
        if ((group & leftover_mask) != 0)
        {
            return NullOpt;
        }

        for (usize i = 0; i < 3 - padding; ++i)
        {
            bytes.Push(static_cast<u8>(group >> (16 - (8 * i))));
        }
    }
    return bytes;
}
} // namespace base64
} // namespace se
