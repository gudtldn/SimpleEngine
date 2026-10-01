#include "SimpleEngine/Core/Serialization/SerializePlan.h"

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/HashSet.h"
#include "SimpleEngine/Core/Container/Stack.h"
#include "SimpleEngine/Utility/HashUtils.h"
#include "SimpleEngine/Utility/Overloaded.h"


namespace se
{
namespace
{
/** 스키마 서술에서 형태를 구분하는 태그. */
enum class ESchemaNode : u8
{
    Leaf = 1,
    Struct = 2,
    Array = 3,
    Set = 4,
    Map = 5,
    Optional = 6,
    Enum = 7,
};

/** 스키마 서술을 FNV-1a로 누적합니다. */
class SchemaHasher
{
public:
    void U64(u64 value)
    {
        detail::FNV1a_U64(hash, value);
    }

    /** 길이를 먼저 넣어 이어진 두 문자열의 경계가 해시에 남게 합니다. */
    void Str(StringView value)
    {
        U64(value.ByteLen());
        hash = hash::FNV(value, hash);
    }

    void Node(ESchemaNode node)
    {
        U64(static_cast<u64>(node));
    }

    [[nodiscard]] u64 Get() const { return hash; }

private:
    u64 hash = detail::FNV_OFFSET_BASIS;
};

/** plan이 직접 가리키는 자식 Plan마다 func를 호출합니다. */
template <typename Fn>
void ForEachChildPlan(const SerializePlan& plan, Fn&& func)
{
    std::visit(Overloaded{
        // 레지스트리가 돌려준 Plan에는 컴파일 중인 단계가 남지 않음
        [](const PendingStep&) { SE_UNREACHABLE(); },
        [](const LeafStep&) {},
        [&](const StructSteps& steps)
        {
            for (const FieldStep& field : steps.fields)
            {
                func(field.plan);
            }
        },
        [&](const ArraySteps& steps) { func(steps.element); },
        [&](const SetSteps& steps) { func(steps.element); },
        [&](const MapSteps& steps)
        {
            func(steps.key);
            func(steps.value);
        },
        [&](const OptionalSteps& steps) { func(steps.inner); },
        [](const EnumStep&) {},
    }, plan.steps);
}

/** root에서 닿는 Plan을 모두 모아 TypeId 순으로 정렬해 돌려줍니다. */
[[nodiscard]] Array<const SerializePlan*> CollectReachablePlans(const SerializePlan& root)
{
    Array<const SerializePlan*> reachable;
    HashSet<TypeId> visited;
    Stack<const SerializePlan*> pending;

    visited.Insert(root.type);
    pending.Push(&root);
    while (const auto plan = pending.Pop())
    {
        reachable.Push(*plan);
        ForEachChildPlan(**plan, [&](const SerializePlan* child)
        {
            if (visited.Insert(child->type))
            {
                pending.Push(child);
            }
        });
    }

    reachable.Sort([](const SerializePlan* lhs, const SerializePlan* rhs) { return lhs->type.Value() < rhs->type.Value(); });
    return reachable;
}

/** plan 자신의 서술을 넣습니다. 자식은 해시가 아니라 TypeId로만 참조합니다. */
void DescribePlan(SchemaHasher& hasher, const SerializePlan& plan)
{
    hasher.U64(plan.type.Value());
    std::visit(Overloaded{
        [](const PendingStep&) { SE_UNREACHABLE(); },
        [&](const LeafStep& leaf)
        {
            // 산술 타입의 폭, 부호, 종류는 TypeId가 정하므로 트레이트 버전만 더 넣음
            hasher.Node(ESchemaNode::Leaf);
            hasher.U64(leaf.ops.format_version);
        },
        [&](const StructSteps& steps)
        {
            hasher.Node(ESchemaNode::Struct);
            hasher.U64(steps.fields.Len());
            for (const FieldStep& field : steps.fields)
            {
                hasher.Str(field.name);
                hasher.U64(field.plan->type.Value());
            }
        },
        [&](const ArraySteps& steps)
        {
            hasher.Node(ESchemaNode::Array);
            hasher.U64(steps.element->type.Value());
        },
        [&](const SetSteps& steps)
        {
            hasher.Node(ESchemaNode::Set);
            hasher.U64(steps.element->type.Value());
        },
        [&](const MapSteps& steps)
        {
            hasher.Node(ESchemaNode::Map);
            hasher.U64(steps.key->type.Value());
            hasher.U64(steps.value->type.Value());
        },
        [&](const OptionalSteps& steps)
        {
            hasher.Node(ESchemaNode::Optional);
            hasher.U64(steps.inner->type.Value());
        },
        [&](const EnumStep& e)
        {
            hasher.Node(ESchemaNode::Enum);
            hasher.U64(static_cast<u64>(e.width));
            hasher.U64(static_cast<u64>(e.is_signed));
            hasher.U64(e.entries.Len());
            for (const EnumEntry& entry : e.entries)
            {
                hasher.U64(static_cast<u64>(entry.value));
                hasher.Str(entry.name);
            }
        },
    }, plan.steps);
}
} // namespace

u64 SerializePlan::SchemaHash() const
{
    SchemaHasher hasher;
    for (const SerializePlan* plan : CollectReachablePlans(*this))
    {
        DescribePlan(hasher, *plan);
    }
    return hasher.Get();
}
} // namespace se
