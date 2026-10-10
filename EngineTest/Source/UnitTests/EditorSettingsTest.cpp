#include "gtest/gtest.h"

#include "SimpleEditor/Config/EditorSettings.h"
#include "SimpleEngine/Core/Serialization/Serializer.h"
#include "SimpleEngine/Core/Serialization/TomlArchive.h"

#include <string>

using namespace se;

// Editor DLL이 정적 초기화에서 등록한 설정 구조체를 Editor DLL 밖(이 실행 파일)에서도 직렬화할 수 있는지 확인

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
