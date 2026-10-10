#include "gtest/gtest.h"

#include "SimpleEditor/Config/EditorSettings.h"
#include "SimpleEngine/Core/Container/ArrayView.h"
#include "SimpleEngine/Core/Container/StringView.h"
#include "SimpleEngine/Core/Reflection/TypeId.h"
#include "SimpleEngine/Core/Reflection/TypeRegistry.h"
#include "SimpleEngine/Core/Serialization/Serializer.h"
#include "SimpleEngine/Core/Serialization/TomlArchive.h"
#include "../../../EngineCore/Include/SimpleEngine/Core/Reflection/Legacy/TypeRegistry.h"

#include <ranges>
#include <string>
#include <string_view>

using namespace se;

// 설정 구조체는 SettingsPanel이 그리는 레거시 리플렉션(TypeRegistry_v1)과 ConfigFile이 읽고 쓰는 리플렉션(TypeRegistry)에
// 모두 등록되므로, 한쪽에만 필드를 추가하는 실수를 잡기 위해 두 등록을 비교
// 두 등록은 Editor DLL이 정적 초기화에서 등록한 정보를 TypeId로 찾아 비교하고,
// Editor DLL 밖(이 실행 파일)에서도 설정 구조체를 직렬화할 수 있는지 확인
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

TEST(EditorSettingsTest, WindowSettingsSerializesFromAnotherModule)
{
    // Editor DLL이 등록한 타입을 이 실행 파일에서 EnsureRegistered하고 Plan을 만들어 씀
    const editor::WindowSettings settings{ .title = "Test Window", .width = 800, .height = 600, .fullscreen = true };

    toml::table table;
    TomlWriter writer(table);
    ASSERT_TRUE(serde::Serialize(writer, settings).HasValue());

    EXPECT_EQ(table["title"].value_exact<std::string>(), "Test Window");
    EXPECT_EQ(table["width"].value_exact<i64>(), 800);
    EXPECT_EQ(table["height"].value_exact<i64>(), 600);
    EXPECT_EQ(table["fullscreen"].value_exact<bool>(), true);
    EXPECT_EQ(table["borderless"].value_exact<bool>(), false);
    EXPECT_EQ(table["resizable"].value_exact<bool>(), true);
}
