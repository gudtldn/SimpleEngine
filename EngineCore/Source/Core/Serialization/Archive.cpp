#include "SimpleEngine/Core/Serialization/Archive.h"

#include <utility>


namespace se
{
Archive::~Archive() = default;

void Archive::SetError(String reason)
{
    has_error = true;
    error_message = std::move(reason);
}
} // namespace se
