#include "Core/Serialization/Archive.h"
#include "Core/Container/String.h"


namespace se
{
Archive::~Archive() = default;
Archive::Archive(Archive&&) noexcept = default;
Archive& Archive::operator=(Archive&&) noexcept = default;

void Archive::SetError(StringView reason)
{
    error_message = std::make_unique<String>(reason);
}

void Archive::ClearError()
{
    error_message.reset();
}

Archive::Archive(EArchiveMode in_mode)
    : mode(in_mode)
{
}
}  // namespace se
