#pragma once
#include <variant>

#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Core/Math/Math.h"
#include "SimpleEngine/Core/Types/Guid.h"
#include "SimpleEngine/Core/Types/StringName.h"
#include "SimpleEngine/Reflection/TypeId.h"


namespace se::asset
{
/** 에셋 파이프라인 내에서 전달되는 범용 속성값 타입 */
using AttributeValue = std::variant<
    bool, float,
    int64, uint64,
    String,
    Vector2f, Vector3f, Vector4f, Matrix4x4f,
    Array<uint8>,
    Guid
>;

/**
 * Asset Import Pipeline의 기본 노드 클래스
 */
class PipelineNodeBase
{
public:
    virtual ~PipelineNodeBase() = default;

    /** 노드의 고유 타입 식별자를 반환합니다. */
    [[nodiscard]] virtual refl::TypeId GetTypeId() const noexcept = 0;

    /** 팩토리 정렬(Topological Sort)을 위해 이 노드가 참조하는 다른 노드들의 ID 반환합니다. */
    virtual void GetFactoryDependencies([[maybe_unused]] Array<Guid>& out_dependencies) const {}

public:
    [[nodiscard]] const Guid& GetUid() const noexcept;
    void SetUid(Guid new_uid) noexcept;

    [[nodiscard]] const Guid& GetParentUid() const noexcept;
    void SetParentUid(const Guid& parent);

    /** 노드의 이름을 반환합니다. */
    [[nodiscard]] const String& GetDisplayName() const noexcept;

    /** 노드의 이름을 설정합니다. */
    void SetDisplayName(const String& new_name) noexcept;

    /** 지정된 키에 해당하는 속성값을 참조로 가져옵니다. */
    template <typename T>
    [[nodiscard]] Optional<const T&> GetAttribute(const StringName& key) const
    {
        return attributes.Find(key).AndThen([](const AttributeValue& attr) -> Optional<const T&>
        {
            if (const T* val = std::get_if<T>(&attr))
            {
                return *val;
            }
            return std::nullopt;
        });
    }

    /** 노드에 새로운 속성을 설정하거나 기존 값을 갱신합니다. */
    template <typename T>
        requires std::constructible_from<AttributeValue, T>
    void SetAttribute(const StringName& key, T&& value)
    {
        attributes.Insert(key, std::forward<T>(value));
    }

private:
    Guid uid;
    Guid parent_uid;
    String display_name;

    HashMap<StringName, AttributeValue> attributes; // TODO: flat_map 나오면 수정
};
}  // namespace se::asset
