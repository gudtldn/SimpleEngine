export module SE.Core:Reflection.TypeId;

import SE.Types;
import SE.Utility;
import std;


namespace se::core::reflection
{
/** 타입 이름과 해시를 제공하는 컴파일타임 타입 식별자입니다. */
export class TypeId
{
public:
    /** 템플릿 타입 T의 TypeId를 반환합니다. */
    template <typename T>
    [[nodiscard]] constexpr static TypeId Get()
    {
        return TypeId{ utility::type_utils::GetTypeSignature<T>() };
    }

    /** 타입 이름을 반환합니다. */
    [[nodiscard]] constexpr std::string_view GetName() const { return type_name; }

    /** 타입 해시를 반환합니다. */
    [[nodiscard]] constexpr uint64 GetHash() const { return type_hash; }

public:
    constexpr bool operator==(const TypeId& other) const
    {
        return type_hash == other.type_hash;
    }

    constexpr bool operator!=(const TypeId& other) const
    {
        return !(*this == other);
    }

    constexpr auto operator<=>(const TypeId& other) const
    {
        return type_hash <=> other.type_hash;
    }

private:
    explicit constexpr TypeId(std::string_view in_type_name)
        : type_name(in_type_name), type_hash(utility::hash::FNV_Hash(in_type_name))
    {
    }

    std::string_view type_name;
    uint64 type_hash = 0;
};
}

template <>
struct std::hash<se::core::reflection::TypeId>
{
    constexpr uint64 operator()(const se::core::reflection::TypeId& id) const noexcept
    {
        return id.GetHash();
    }
};
