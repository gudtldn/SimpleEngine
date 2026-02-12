#include "Core/Serialization/Archive.h"


namespace se
{
void Archive::SetError(StringView reason)
{
    has_error = true;
    error_message = reason;
}

void Archive::ClearError()
{
    has_error = false;
    error_message = "";
}
}  // namespace se
