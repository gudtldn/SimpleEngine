#include "Core/Container/StringView.h"
#include "Core/Container/String.h"


namespace se
{
StringView::StringView(const String& str) noexcept
    : data_ptr(str.CStr())
    , data_len(str.ByteLen())
{
}

String StringView::ToString() const
{
    return String(data_ptr, data_len);
}
}  // namespace se
