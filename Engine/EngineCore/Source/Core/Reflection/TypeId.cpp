module SimpleEngine.Core.Reflection;
import :TypeId;

import SimpleEngine.Core.Hash;


namespace se::reflection
{
constexpr TypeId::TypeId(std::string_view in_type_name)
    : type_name(in_type_name), type_hash(hash::FowlerNollVoHash(in_type_name))
{
}
}
