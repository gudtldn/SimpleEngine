#include "gtest/gtest.h"

#include "SimpleEngine/Core/Container/StringView.h"
#include "SimpleEngine/Core/Reflection/TypeId.h"
#include "SimpleEngine/Core/Reflection/TypeRegistry.h"
#include "SimpleEngine/ECS/ECSRegistry.h"
#include "SimpleEngine/ECS/WorldFileSkip.h"
#include "../../../EngineCore/Include/SimpleEngine/Core/Reflection/Legacy/TypeRegistry.h"

#include <algorithm>
#include <ranges>
#include <string>
#include <string_view>

using namespace se;

// 컴포넌트는 DetailPanel과 ECSRegistry가 쓰는 레거시 리플렉션(TypeRegistry_v1)과 월드 파일이 읽고 쓰는 리플렉션(TypeRegistry)에
// 모두 등록되므로, 한쪽에만 필드를 추가하거나 저장하지 않는 표시를 한쪽에만 붙이는 실수를 잡기 위해 두 등록을 비교
namespace
{
/** 이름들을 ", "로 이어 붙입니다. 실패 메시지에 두 목록이 그대로 보이도록 문자열로 비교합니다. */
template <std::ranges::input_range Names>
[[nodiscard]] std::string JoinNames(Names&& names)
{
    std::string joined;
    for (const StringView name : names)
    {
        if (!joined.empty())
        {
            joined += ", ";
        }
        joined += std::string_view{ name };
    }
    return joined;
}
} // namespace


TEST(ComponentRegistrationTest, BothRegistrationsAgreeForEveryComponent)
{
    const auto& component_ops = ECSRegistry::Get().GetComponentOpsMap();
    ASSERT_FALSE(component_ops.IsEmpty());

    for (const auto& [legacy_id, ops] : component_ops)
    {
        const auto legacy_info = TypeRegistry_v1::Get().Find(legacy_id);
        ASSERT_TRUE(legacy_info.HasValue());
        SCOPED_TRACE(std::string_view{ legacy_info->name });

        const auto info = TypeRegistry::Get().Find(ops.type);
        ASSERT_TRUE(info.HasValue()) << "The component is not registered with SE_REFLECT_BEGIN.";

        // 월드 파일에 쓰지 않는 컴포넌트는 두 등록 모두에 표시되어야 함
        const bool is_skipped = info->HasAnnotation<WorldFileSkip>();
        EXPECT_EQ(is_skipped, legacy_info->flags.IsSet(ETypeFlags_v1::Transient))
            << "WorldFileSkip in SE_REFLECT_BEGIN and meta::Transient in SE_BEGIN_REFLECT_V1 must agree.";
        if (is_skipped)
        {
            continue;
        }

        // 저장하는 컴포넌트는 필드 이름과 순서가 모두 같아야 함
        const auto struct_info = info->AsStruct();
        ASSERT_TRUE(struct_info.HasValue());
        EXPECT_EQ(
            JoinNames(struct_info->fields | std::views::transform(&FieldInfo::name)),
            JoinNames(legacy_info->properties | std::views::transform(&PropertyInfo_v1::name))
        );
    }
}
