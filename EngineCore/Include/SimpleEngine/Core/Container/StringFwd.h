#pragma once

namespace se
{
template <typename Allocator> class BaseString;
template <typename T> class DefaultAllocator;
using String = BaseString<DefaultAllocator<char>>;

class StringView;
} // namespace se
