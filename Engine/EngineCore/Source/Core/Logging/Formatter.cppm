export module SimpleEngine.Logging:Formatter;
import std;


// std::u8string_view에 대한 std::formatter 특수화
template <>
struct std::formatter<std::u8string_view, char> : std::formatter<std::string_view>
{
    auto format(std::u8string_view u8sv, std::format_context& ctx) const
    {
        // u8string_view를 string_view로 변환 (바이트 시퀀스로 재해석)
        const std::string_view sv(reinterpret_cast<const char*>(u8sv.data()), u8sv.size());
        return std::formatter<std::string_view>::format(sv, ctx);
    }
};

// std::u8string에 대한 std::formatter 특수화
template <>
struct std::formatter<std::u8string, char> : std::formatter<std::u8string_view>
{
    auto format(const std::u8string& u8s, std::format_context& ctx) const
    {
        return std::formatter<std::u8string_view>::format(std::u8string_view(u8s), ctx);
    }
};

// const char8_t* 에 대한 std::formatter 특수화
template <>
struct std::formatter<const char8_t*, char> : std::formatter<std::u8string_view>
{
    auto format(const char8_t* u8s_ptr, std::format_context& ctx) const
    {
        if (!u8s_ptr)
        {
            return std::formatter<std::u8string_view>::format(std::u8string_view{}, ctx);
        }
        return std::formatter<std::u8string_view>::format(std::u8string_view(u8s_ptr), ctx);
    }
};

// char8_t* 에 대한 std::formatter 특수화
template <>
struct std::formatter<char8_t*, char> : std::formatter<const char8_t*>
{
    // ReSharper disable once CppParameterMayBeConstPtrOrRef
    auto format(char8_t* u8s_ptr, std::format_context& ctx) const
    {
        return std::formatter<const char8_t*>::format(u8s_ptr, ctx);
    }
};

// const char8_t[N] (예: u8"문자열 리터럴")에 대한 std::formatter 특수화
template <size_t N>
struct std::formatter<const char8_t[N], char> : std::formatter<const char8_t*>
{
};

// char8_t[N]에 대한 std::formatter 특수화
template <size_t N>
struct std::formatter<char8_t[N], char> : std::formatter<const char8_t*>
{
};
