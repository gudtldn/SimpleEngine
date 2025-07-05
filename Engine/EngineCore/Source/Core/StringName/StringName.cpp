module SimpleEngine.Core;
import :StringName;
import :StringName.StringNamePool;

import SimpleEngine.Utility;

// TODO: 스레드 동기화 로직 추가, shared_mutex
namespace se::core::string_name
{
StringName StringName::None = StringName{};


StringName::StringName(const char8* in_str)
    : StringName(std::u8string_view(in_str))
{
}

StringName::StringName(std::u8string_view in_str)
{
    // TODO: StringNamePool에서 가져오기

    const std::u8string lower_case_str = utility::string_utils::ToU8LowerCase(in_str);
    if (lower_case_str == u8"none")
    {
        *this = None;
        return;
    }

    StringNamePool& pool = StringNamePool::Get();
    display_hash = pool.FindOrStore(in_str);
    comparison_hash = pool.Resolve(display_hash).comparison_hash;
}

std::u8string StringName::ToString() const
{
    StringNamePool& pool = StringNamePool::Get();

    // TODO: Implements this
    std::unreachable();
}
}
