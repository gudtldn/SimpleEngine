#pragma once

#include "SimpleEditor/EditorCommon.h"

#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/Math/Math.h"
#include "SimpleEngine/Core/Types/Guid.h"
#include "SimpleEngine/Core/Types/StringName.h"

#include <concepts>


namespace se::editor
{
/**
 * 에셋 파이프라인 시스템에서 속성을 식별하기 위한 Key 클래스
 */
class SE_EDITOR_API AttributeKey
{
public:
    AttributeKey(String in_string)
        : key_str(std::move(in_string))
    {
    }

    AttributeKey(const char* in_str)
        : key_str(in_str)
    {
    }

    AttributeKey(const StringName& in_name)
        : key_str(in_name.CStr())
    {
    }

public:
    [[nodiscard]] const String& ToString() const { return key_str; }

    [[nodiscard]] bool operator==(const AttributeKey& other) const = default;
    [[nodiscard]] auto operator<=>(const AttributeKey& other) const = default;

private:
    String key_str;
};
} // namespace se::editor

// std::hash specialization
template <>
struct std::hash<se::editor::AttributeKey>
{
    size_t operator()(const se::editor::AttributeKey& key) const noexcept
    {
        return std::hash<se::String>{}(key.ToString());
    }
};

namespace se::editor
{
/** 에셋 파이프라인 내에서 전달되는 범용 속성값 타입 */
using AttributeValue = std::variant<
    bool, double,
    int64, uint64,
    Vector2, Vector3, Vector4, Matrix4x4,
    String, Array<uint8>,
    Guid
>;

/**
 * 파이프라인 노드(Node)들이 데이터를 저장하는 제네릭 컨테이너
 */
class SE_EDITOR_API AttributeStorage
{
public:
    /** 지정된 키에 해당하는 속성값을 가져옵니다. */
    template <typename T>
    [[nodiscard]] Optional<traits::ParamType<T>> GetAttribute(const AttributeKey& key) const
    {
        using ReturnType = traits::ParamType<T>;
        return attributes.Find(key).AndThen([](const AttributeValue& attr) -> Optional<ReturnType>
        {
            return std::visit([]<typename Arg>(const Arg& arg) -> Optional<ReturnType>
            {
                using StoredType = std::decay_t<Arg>;

                // 타입이 정확히 일치하는 경우
                if constexpr (std::same_as<StoredType, T>)
                {
                    return Optional<ReturnType>(arg);
                }
                // 타입은 다르지만 T타입으로 형변환이 가능한 경우 (+참조 반환이 아니어야 함)
                else if constexpr (std::convertible_to<StoredType, T> && !std::is_reference_v<ReturnType>)
                {
                    return Optional<ReturnType>(static_cast<T>(arg));
                }
                // 변환 불가능
                else
                {
                    return Optional<ReturnType>{};
                }
            }, attr);
        });
    }

    /** 지정된 키에 해당하는 속성값을 설정하거나 기존 값을 갱신합니다. */
    template <typename T>
        requires std::constructible_from<AttributeValue, T>
    void SetAttribute(const AttributeKey& key, T&& value)
    {
        if constexpr (std::same_as<std::decay_t<T>, const char*>)
        {
            attributes.Insert(key, String(value));
        }
        else
        {
            attributes.Insert(key, std::forward<T>(value));
        }
    }

    /** 지정된 키에 해당하는 속성이 존재하는지 확인 합니다. */
    [[nodiscard]] bool HasAttribute(const AttributeKey& key) const
    {
        return attributes.Contains(key);
    }

private:
    HashMap<AttributeKey, AttributeValue> attributes;
};
} // namespace se::editor
