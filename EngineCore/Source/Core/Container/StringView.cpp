#include "SimpleEngine/Core/Container/StringView.h"
#include "SimpleEngine/Core/Container/String.h"


namespace se
{
StringView::StringView(const String& str) noexcept
    : data_ptr(str.CStr())
    , data_len(str.ByteLen())
{
}

String StringView::ToString() const
{
    return { data_ptr, data_len };
}
} // namespace se
