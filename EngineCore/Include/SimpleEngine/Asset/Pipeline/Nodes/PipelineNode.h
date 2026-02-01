#pragma once
#include "SimpleEngine/Asset/Pipeline/Types/AttributeStorage.h"
#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/Types/Guid.h"
#include "SimpleEngine/Core/Reflection/TypeId.h"


namespace se::asset
{
/**
 * Asset Import Pipeline의 기본 노드 클래스
 */
class SE_CORE_API PipelineBaseNode
{
public:
    virtual ~PipelineBaseNode() = default;

    /** 노드의 고유 타입 식별자를 반환합니다. */
    [[nodiscard]] virtual TypeId GetTypeId() const = 0;

    /** 팩토리 정렬(Topological Sort)을 위해 이 노드가 참조하는 다른 노드들의 ID 반환합니다. */
    virtual void GetFactoryDependencies(Array<Guid>& out_dependencies) const
    {
        if (parent_uid.IsValid())
        {
            out_dependencies.Push(parent_uid);
        }
    }

public:
    [[nodiscard]] FORCE_INLINE const Guid& GetUid() const { return self_uid; }
    FORCE_INLINE void SetUid(const Guid& new_uid) { self_uid = new_uid; }

    [[nodiscard]] FORCE_INLINE const Guid& GetParentUid() const { return parent_uid; }
    FORCE_INLINE void SetParentUid(const Guid& parent) { parent_uid = parent; }

    [[nodiscard]] FORCE_INLINE const String& GetDisplayName() const { return display_name; }
    FORCE_INLINE void SetDisplayName(const String& new_name) { display_name = new_name; }

    [[nodiscard]] FORCE_INLINE AttributeStorage& GetAttributes() { return attributes; }
    [[nodiscard]] FORCE_INLINE const AttributeStorage& GetAttributes() const { return attributes; }

protected:
    Guid self_uid;
    Guid parent_uid;
    String display_name;

    AttributeStorage attributes;
};

/**
 * CRTP 패턴을 활용한 Pipeline 기본 노드 템플릿 클래스
 * @tparam Derived 실제 이 클래스를 상속받는 하위 클래스 타입
 */
template <typename Derived>
class PipelineNode : public PipelineBaseNode
{
    friend Derived;

protected:
    PipelineNode() = default;
    virtual ~PipelineNode() override = default;

public:
    [[nodiscard]] virtual TypeId GetTypeId() const override final
    {
        static_assert(std::derived_from<Derived, PipelineNode>, "CRTP Error: Derived class must inherit from PipelineNode<Derived>");
        return TypeId::Get<Derived>();
    }
};
}  // namespace se::asset
