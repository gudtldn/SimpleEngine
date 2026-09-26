#include "gtest/gtest.h"

#include "SimpleEditor/Config/EditorSettings.h"
#include "SimpleEngine/Core/Container/ArrayView.h"
#include "SimpleEngine/Core/Container/StringView.h"
#include "SimpleEngine/Core/Reflection/TypeId.h"
#include "SimpleEngine/Core/Reflection/TypeRegistry.h"
#include "../../../EngineCore/Include/SimpleEngine/Core/Reflection/Legacy/TypeRegistry.h"

#include <ranges>
#include <string>
#include <string_view>

using namespace se;

// 설정 구조체는 SettingsPanel이 그리는 레거시 리플렉션(TypeRegistry_v1)과 ConfigFile이 읽고 쓰는 리플렉션(TypeRegistry)에
// 모두 등록되므로, 한쪽에만 필드를 추가하는 실수를 잡기 위해 두 등록을 비교
// Editor DLL의 타입이라 등록 템플릿(EnsureRegistered, SerializePlan::Of 등)은 인스턴스화하지 않고,
// Editor DLL이 정적 초기화에서 등록한 정보를 TypeId로 찾음
namespace
{
/** 설정 타입 하나를 두 레지스트리에서 찾을 TypeId */
struct SettingsTypeIds
{
    TypeId id;
    TypeId_v1 legacy_id;
};

/** T의 두 TypeId를 만듭니다. 둘 다 타입 이름의 해시라 등록 템플릿을 인스턴스화하지 않습니다. */
template <typename T>
[[nodiscard]] SettingsTypeIds IdsOf()
{
    return { .id = TypeId::Of<T>(), .legacy_id = TypeId_v1::Of<T>() };
}

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


TEST(EditorSettingsTest, BothRegistrationsListSameFields)
{
    const SettingsTypeIds settings_types[] = {
        IdsOf<editor::WindowSettings>(),
        IdsOf<editor::EditorUISettings>(),
        IdsOf<editor::ConsoleSettings>(),
        IdsOf<editor::PerformanceSettings>(),
        IdsOf<editor::GraphicsSettings>(),
        IdsOf<editor::AssetScanSettings>(),
    };

    for (const SettingsTypeIds& type : settings_types)
    {
        const auto legacy_info = TypeRegistry_v1::Get().Find(type.legacy_id);
        ASSERT_TRUE(legacy_info.HasValue());
        SCOPED_TRACE(std::string_view{ legacy_info->name });

        const auto info = TypeRegistry::Get().Find(type.id);
        ASSERT_TRUE(info.HasValue()) << "The type is not registered with SE_REFLECT_BEGIN.";
        const auto struct_info = info->AsStruct();
        ASSERT_TRUE(struct_info.HasValue());

        // 이름과 순서가 모두 같아야 함
        EXPECT_EQ(
            JoinNames(struct_info->fields | std::views::transform(&FieldInfo::name)),
            JoinNames(legacy_info->properties | std::views::transform(&PropertyInfo_v1::name))
        );
    }
}

TEST(EditorSettingsTest, BothRegistrationsListSamePresentModeNames)
{
    const auto legacy_info = TypeRegistry_v1::Get().Find(TypeId_v1::Of<editor::EPresentMode>());
    ASSERT_TRUE(legacy_info.HasValue());
    ASSERT_NE(legacy_info->enum_entries, nullptr);

    const EnumEntry_v1* legacy_data = nullptr;
    usize legacy_count = 0;
    legacy_info->enum_entries(legacy_data, legacy_count);
    const ArrayView<const EnumEntry_v1> legacy_entries(legacy_data, legacy_count);

    const auto info = TypeRegistry::Get().Find(TypeId::Of<editor::EPresentMode>());
    ASSERT_TRUE(info.HasValue()) << "The enum is not registered with SE_REFLECT_ENUM_BEGIN.";
    const auto enum_info = info->AsEnum();
    ASSERT_TRUE(enum_info.HasValue());

    // 레거시는 값을 자동으로 모으고 SE_REFLECT_ENUM_BEGIN은 SE_ENUM_VALUE로 적은 것만 가지므로, 값을 추가할 때 빠뜨리면 실패
    EXPECT_EQ(
        JoinNames(enum_info->entries | std::views::transform(&EnumEntry::name)),
        JoinNames(legacy_entries | std::views::transform(&EnumEntry_v1::name))
    );
}
