#include "SimpleEngine/Core/Serialization/Legacy/Archive.h"
#include "SimpleEngine/Core/Container/String.h"


namespace se
{
Archive_v1::~Archive_v1() = default;
Archive_v1::Archive_v1(Archive_v1&&) noexcept = default;
Archive_v1& Archive_v1::operator=(Archive_v1&&) noexcept = default;

void Archive_v1::SetError(String reason)
{
    error_message = std::make_unique<String>(std::move(reason));
}

void Archive_v1::ClearError()
{
    error_message.reset();
}

Archive_v1::Archive_v1(EArchiveMode_v1 in_mode)
    : mode(in_mode)
{
}
} // namespace se
