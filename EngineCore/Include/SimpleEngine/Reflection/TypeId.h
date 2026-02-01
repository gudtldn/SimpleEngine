#pragma once
#include "SimpleEngine/Reflection/TypeSignature.h"
#include "SimpleEngine/Utility/Hash.h"


namespace se
{
// forward declaration
class StringName;

/**
 * 타입 이름과 해시를 제공하는 컴파일타임 타입 식별자입니다.
 */
class TypeId
{
public:
    constexpr TypeId() = default;

    /** 템플릿 타입 T의 TypeId를 반환합니다. */
    template <typename T>
    [[nodiscard]] constexpr static TypeId Get()
    {
        if constexpr (traits::IsFunctionType<T>)
        {
            constexpr auto signature = GetRawTypeName<T>();
            return TypeId{ signature, signature };
        }
        else
        {
            using CleanType = std::remove_cvref_t<T>;
            return TypeId{ GetFullTypeName<CleanType>(), GetRawTypeName<CleanType>() };
        }
    }

    /**
     * 해시를 이용해 TypeId를 생성합니다.
     * 만약 TypeRegistry에 없는 Hash면 빈 TypeId를 반환합니다.
     */
    [[nodiscard]] static TypeId FromHash(uint64 in_hash);

    /**
     * 타입 이름을 이용해 TypeId를 생성합니다.
     * 만약 TypeRegistry에 없는 이름이면 빈 TypeId를 반환합니다.
     * @note 이름은 namespace 포함 타입 이름입니다. (예: se::asset::Texture2D)
     */
    [[nodiscard]] static TypeId FromName(const StringName& in_type_name);

public:
    /** 타입 이름을 반환합니다. */
    [[nodiscard]] constexpr StringView GetName() const { return type_name; }

    /** 타입 해시를 반환합니다. */
    [[nodiscard]] constexpr uint64 GetHash() const { return type_hash; }

    /** TypeId가 올바른지 확인합니다. */
    [[nodiscard]] constexpr bool IsValid() const { return type_hash != 0; }

public:
    [[nodiscard]] constexpr bool operator==(const TypeId& other) const { return type_hash == other.type_hash; }
    [[nodiscard]] constexpr auto operator<=>(const TypeId& other) const { return type_hash <=> other.type_hash; }
    [[nodiscard]] explicit constexpr operator bool() const { return IsValid(); }

private:
    constexpr TypeId(StringView in_type_name, StringView in_type_hash)
        : type_name(in_type_name)
        , type_hash(utility::FNV_Hash(in_type_hash))
    {
    }

    constexpr TypeId(StringView in_type_name, uint64 in_hash)
        : type_name(in_type_name)
        , type_hash(in_hash)
    {
    }

    StringView type_name;
    uint64 type_hash = 0;
};
}  // namespace se

template <>
struct std::hash<se::TypeId>
{
    constexpr size_t operator()(const se::TypeId& id) const noexcept
    {
        return static_cast<size_t>(id.GetHash());
    }
};
