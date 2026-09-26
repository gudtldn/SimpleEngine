#include "../UnitTestEnvironment.h"
#include "gtest/gtest.h"

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/Config/ConfigFile.h"
#include "SimpleEngine/Core/FileSystem/FileSystem.h"
#include "SimpleEngine/Core/FileSystem/VFS.h"
#include "SimpleEngine/Core/Reflection/ReflectMacros.h"
#include "SimpleEngine/Core/Types/VPath.h"


using namespace se;


// ============================================================================
//  테스트용 설정 구조체 정의
// ============================================================================
namespace config_test
{
/** [window] 섹션에 대응하는 설정 구조체 */
struct WindowSettings
{
    u32 width = 800;
    u32 height = 600;
    bool fullscreen = true;
    String title = "Default Title";
    f32 scale = 1.0f;

    bool operator==(const WindowSettings&) const = default;
};

/** [graphics] 섹션에 대응하는 설정 구조체 */
struct GraphicsSettings
{
    bool vsync = false;
    i32 max_fps = 60;
    Array<String> shaders;

    bool operator==(const GraphicsSettings&) const = default;
};

/** [logging] 섹션에 대응하는 설정 구조체 */
struct LoggingSettings
{
    String level = "info";
    bool output_to_file = false;
    String log_file_path;

    bool operator==(const LoggingSettings&) const = default;
};

/** 저장하지 않는 필드(transient_val)를 포함하는 테스트 구조체 */
struct TransientSettings
{
    i32 saved_val = 0;
    i32 transient_val = 0;

    bool operator==(const TransientSettings&) const = default;
};

/** 빈 구조체 */
struct EmptySettings
{
    bool operator==(const EmptySettings&) const = default;
};

/** 컨테이너 필드를 가진 구조체 */
struct ContainerSettings
{
    Array<i32> numbers;

    bool operator==(const ContainerSettings&) const = default;
};

/** 맵 필드를 가진 구조체. TOML 아카이브가 아직 맵을 쓰지 못하므로 쓰기 실패 검증에 씁니다. */
struct MapSettings
{
    HashMap<String, f32> scores;
};

/** 루트 레벨 설정 (섹션 없이 최상위에 놓이는 키들) */
struct RootSettings
{
    String title;
    String engine_version;

    bool operator==(const RootSettings&) const = default;
};

/** 이름을 등록한 enum */
enum class EPresentMode : u8
{
    Mailbox,
    VSync,
    Immediate,
};

/** 레거시 형식(enum은 정수, f32는 f64로 확장된 값) 읽기 검증용 구조체 */
struct LegacyFormatSettings
{
    EPresentMode present_mode = EPresentMode::Mailbox;
    f32 busy_wait_ratio = 0.5f;
};
} // namespace config_test


// ============================================================================
//  리플렉션 등록
// ============================================================================
using namespace config_test;

SE_DECLARE_REFLECTION(config_test::WindowSettings)
SE_DECLARE_REFLECTION(config_test::GraphicsSettings)
SE_DECLARE_REFLECTION(config_test::LoggingSettings)
SE_DECLARE_REFLECTION(config_test::TransientSettings)
SE_DECLARE_REFLECTION(config_test::EmptySettings)
SE_DECLARE_REFLECTION(config_test::ContainerSettings)
SE_DECLARE_REFLECTION(config_test::MapSettings)
SE_DECLARE_REFLECTION(config_test::RootSettings)
SE_DECLARE_REFLECTION(config_test::EPresentMode)
SE_DECLARE_REFLECTION(config_test::LegacyFormatSettings)

SE_REFLECT_BEGIN(config_test::WindowSettings)
    SE_FIELD(width)
    SE_FIELD(height)
    SE_FIELD(fullscreen)
    SE_FIELD(title)
    SE_FIELD(scale)
SE_REFLECT_END()

SE_REFLECT_BEGIN(config_test::GraphicsSettings)
    SE_FIELD(vsync)
    SE_FIELD(max_fps)
    SE_FIELD(shaders)
SE_REFLECT_END()

SE_REFLECT_BEGIN(config_test::LoggingSettings)
    SE_FIELD(level)
    SE_FIELD(output_to_file)
    SE_FIELD(log_file_path)
SE_REFLECT_END()

// transient_val은 등록하지 않아 저장되지 않음
SE_REFLECT_BEGIN(config_test::TransientSettings)
    SE_FIELD(saved_val)
SE_REFLECT_END()

SE_REFLECT_BEGIN(config_test::EmptySettings)
SE_REFLECT_END()

SE_REFLECT_BEGIN(config_test::ContainerSettings)
    SE_FIELD(numbers)
SE_REFLECT_END()

SE_REFLECT_BEGIN(config_test::MapSettings)
    SE_FIELD(scores)
SE_REFLECT_END()

SE_REFLECT_BEGIN(config_test::RootSettings)
    SE_FIELD(title)
    SE_FIELD(engine_version)
SE_REFLECT_END()

SE_REFLECT_ENUM_BEGIN(config_test::EPresentMode)
    SE_ENUM_VALUE(Mailbox)
    SE_ENUM_VALUE(VSync)
    SE_ENUM_VALUE(Immediate)
SE_REFLECT_ENUM_END()

SE_REFLECT_BEGIN(config_test::LegacyFormatSettings)
    SE_FIELD(present_mode)
    SE_FIELD(busy_wait_ratio)
SE_REFLECT_END()


// ============================================================================
//  Fixture
// ============================================================================
class ConfigFileTest : public ::testing::Test
{
protected:
    virtual void SetUp() override
    {
        ConfigFile::InvalidateAllCaches();
        auto result = ConfigFile::Load(test_toml_path);
        ASSERT_TRUE(result.HasValue()) << "Failed to load test config: " << result.Error().CStr();
        config = std::move(result).Value();
    }

    ConfigFile config;

    static const VPath test_toml_path;
    static const VPath non_existent_path;
    static const VPath invalid_toml_path;
    static const VPath save_test_path;
    static const VPath legacy_toml_path;
};

const VPath ConfigFileTest::test_toml_path = "Config://ConfigTest.toml";
const VPath ConfigFileTest::non_existent_path = "Config://NonExistent.toml";
const VPath ConfigFileTest::invalid_toml_path = "Config://Invalid.toml";
const VPath ConfigFileTest::save_test_path = "Config://ConfigFileSaveTest.toml";
const VPath ConfigFileTest::legacy_toml_path = "Config://ConfigFileLegacyTest.toml";


// ============================================================================
//  Load / Save 기본 테스트
// ============================================================================
TEST_F(ConfigFileTest, LoadExistingFileSucceeds)
{
    auto result = ConfigFile::Load(test_toml_path);
    EXPECT_TRUE(result.HasValue());
}

TEST_F(ConfigFileTest, LoadNonExistentFileFails)
{
    auto result = ConfigFile::Load(non_existent_path);
    EXPECT_FALSE(result.HasValue());
    EXPECT_FALSE(result.Error().IsEmpty());
}

TEST_F(ConfigFileTest, LoadInvalidTomlFails)
{
    const auto physical_path = VFS::ToPath(invalid_toml_path);
    fs::WriteString(physical_path, "this = is not valid toml' syntax");

    auto result = ConfigFile::Load(invalid_toml_path);
    EXPECT_FALSE(result.HasValue());

    fs::Remove(physical_path);
}

TEST_F(ConfigFileTest, DefaultConstructedIsEmpty)
{
    ConfigFile empty;
    EXPECT_TRUE(empty.IsEmpty());
}

TEST_F(ConfigFileTest, LoadedConfigIsNotEmpty)
{
    EXPECT_FALSE(config.IsEmpty());
}


// ============================================================================
//  GetSection 테스트
// ============================================================================
TEST_F(ConfigFileTest, GetSectionDeserializesWindowSettings)
{
    auto window = config.GetSection<WindowSettings>("window");

    EXPECT_EQ(window.width, 1280u);
    EXPECT_EQ(window.height, 720u);
    EXPECT_EQ(window.fullscreen, false);
    EXPECT_EQ(window.title, "SimpleEngine Editor");
    EXPECT_FLOAT_EQ(window.scale, 1.5f);
}

TEST_F(ConfigFileTest, GetSectionDeserializesGraphicsSettings)
{
    auto gfx = config.GetSection<GraphicsSettings>("graphics");

    EXPECT_EQ(gfx.vsync, true);
    EXPECT_EQ(gfx.max_fps, 144);
    ASSERT_EQ(gfx.shaders.Len(), 2u);
    EXPECT_EQ(gfx.shaders[0], "default.vert");
    EXPECT_EQ(gfx.shaders[1], "default.frag");
}

TEST_F(ConfigFileTest, GetSectionDeserializesLoggingSettings)
{
    auto logging = config.GetSection<LoggingSettings>("logging");

    EXPECT_EQ(logging.level, "debug");
    EXPECT_EQ(logging.output_to_file, true);
    EXPECT_EQ(logging.log_file_path, "engine.log");
}

TEST_F(ConfigFileTest, GetMissingSectionReturnsDefaults)
{
    auto window = config.GetSection<WindowSettings>("does_not_exist");

    // 구조체 기본값이 반환되어야 함
    EXPECT_EQ(window.width, 800u);
    EXPECT_EQ(window.height, 600u);
    EXPECT_EQ(window.fullscreen, true);   // 기본값
    EXPECT_EQ(window.title, "Default Title");
    EXPECT_FLOAT_EQ(window.scale, 1.0f);
}

TEST_F(ConfigFileTest, GetSectionFromRootTable)
{
    auto root = config.GetSection<RootSettings>();

    EXPECT_EQ(root.title, "TOML Example Config for SimpleEngine Tests");
    EXPECT_EQ(root.engine_version, "0.1.0-alpha");
}

TEST_F(ConfigFileTest, GetSectionPartialMatch_MissingFieldsKeepDefaults)
{
    // LoggingSettings를 "window" 섹션으로 읽으면 필드가 매칭되지 않아 기본값 유지
    auto logging = config.GetSection<LoggingSettings>("window");

    EXPECT_EQ(logging.level, "info");            // 기본값
    EXPECT_EQ(logging.output_to_file, false);    // 기본값
    EXPECT_TRUE(logging.log_file_path.IsEmpty()); // 기본값
}

TEST_F(ConfigFileTest, GetSectionFailureReturnsDefaults)
{
    ConfigFile new_config;
    new_config.SetValue("window.width", 1920);
    new_config.SetValue("window.title", 42); // 문자열이어야 하는 값

    // width를 읽은 뒤 title에서 실패하므로, 읽다 만 값 대신 기본 생성된 구조체를 받아야 함
    const auto window = new_config.GetSection<WindowSettings>("window");
    EXPECT_EQ(window, WindowSettings{});
}


// ============================================================================
//  SetSection 테스트
// ============================================================================
TEST_F(ConfigFileTest, SetSectionAndGetBack)
{
    ConfigFile new_config;

    WindowSettings expected;
    expected.width = 1920;
    expected.height = 1080;
    expected.fullscreen = true;
    expected.title = "Test Window";
    expected.scale = 2.0f;

    new_config.SetSection(expected, "window");

    auto actual = new_config.GetSection<WindowSettings>("window");
    EXPECT_EQ(actual, expected);
}

TEST_F(ConfigFileTest, SetSectionOverwritesExisting)
{
    // 기존 window 섹션을 읽고 수정 후 다시 설정
    auto window = config.GetSection<WindowSettings>("window");
    ASSERT_EQ(window.width, 1280u);

    window.width = 3840;
    window.height = 2160;
    config.SetSection(window, "window");

    auto reloaded = config.GetSection<WindowSettings>("window");
    EXPECT_EQ(reloaded.width, 3840u);
    EXPECT_EQ(reloaded.height, 2160u);
    // 변경하지 않은 필드는 유지
    EXPECT_EQ(reloaded.fullscreen, false);
    EXPECT_EQ(reloaded.title, "SimpleEngine Editor");
}

TEST_F(ConfigFileTest, SetSectionToRoot)
{
    ConfigFile new_config;

    RootSettings root;
    root.title = "My Engine";
    root.engine_version = "2.0.0";

    new_config.SetSection(root);

    auto actual = new_config.GetSection<RootSettings>();
    EXPECT_EQ(actual.title, "My Engine");
    EXPECT_EQ(actual.engine_version, "2.0.0");
}

TEST_F(ConfigFileTest, SetSectionWithContainers)
{
    ConfigFile new_config;

    ContainerSettings expected;
    expected.numbers = { 10, 20, 30, 40 };

    new_config.SetSection(expected, "data");

    auto actual = new_config.GetSection<ContainerSettings>("data");
    EXPECT_EQ(actual.numbers, expected.numbers);
}

TEST_F(ConfigFileTest, SetSectionFailureKeepsExistingSection)
{
    ConfigFile new_config;
    new_config.SetValue("data.kept", 7);

    // TOML 아카이브가 아직 맵을 쓰지 못해 실패하므로 기존 섹션이 그대로 남아야 함
    MapSettings settings;
    settings.scores.Insert("alice", 95.5f);
    new_config.SetSection(settings, "data");

    EXPECT_EQ(new_config.GetValue<i64>("data.kept").Value(), 7);
    EXPECT_FALSE(new_config.GetValue<f64>("data.scores.alice").HasValue());
}

TEST_F(ConfigFileTest, TransientFieldNotSerialized)
{
    ConfigFile new_config;

    TransientSettings original;
    original.saved_val = 42;
    original.transient_val = 999;

    new_config.SetSection(original, "settings");

    auto loaded = new_config.GetSection<TransientSettings>("settings");
    EXPECT_EQ(loaded.saved_val, 42);
    EXPECT_EQ(loaded.transient_val, 0);  // 등록하지 않은 필드 -> 직렬화되지 않아 기본값
}

TEST_F(ConfigFileTest, EmptyStructRoundTrip)
{
    ConfigFile new_config;

    EmptySettings empty;
    new_config.SetSection(empty, "empty");

    auto loaded = new_config.GetSection<EmptySettings>("empty");
    EXPECT_EQ(loaded, empty);
}

TEST_F(ConfigFileTest, MultipleSectionsIndependent)
{
    ConfigFile new_config;

    WindowSettings window;
    window.width = 1920;
    window.height = 1080;

    GraphicsSettings gfx;
    gfx.vsync = true;
    gfx.max_fps = 240;
    gfx.shaders = { "pbr.vert", "pbr.frag" };

    new_config.SetSection(window, "window");
    new_config.SetSection(gfx, "graphics");

    auto loaded_window = new_config.GetSection<WindowSettings>("window");
    auto loaded_gfx = new_config.GetSection<GraphicsSettings>("graphics");

    EXPECT_EQ(loaded_window.width, 1920u);
    EXPECT_EQ(loaded_gfx.vsync, true);
    EXPECT_EQ(loaded_gfx.max_fps, 240);
    ASSERT_EQ(loaded_gfx.shaders.Len(), 2u);
    EXPECT_EQ(loaded_gfx.shaders[0], "pbr.vert");
}


// ============================================================================
//  GetValue 테스트 (개별 스칼라 접근)
// ============================================================================
TEST_F(ConfigFileTest, GetValuePrimitiveTypes)
{
    EXPECT_EQ(config.GetValue<bool>("a_boolean").Value(), true);
    EXPECT_EQ(config.GetValue<i64>("an_integer").Value(), 42);
    EXPECT_NEAR(config.GetValue<f64>("a_float").Value(), 3.14159, 1e-5);
    EXPECT_EQ(config.GetValue<String>("a_string").Value(), "Hello, TOML!");
}

TEST_F(ConfigFileTest, GetValueNestedKey)
{
    EXPECT_EQ(config.GetValue<i64>("window.width").Value(), 1280);
    EXPECT_EQ(config.GetValue<bool>("window.fullscreen").Value(), false);
    EXPECT_EQ(config.GetValue<String>("logging.level").Value(), "debug");
}

TEST_F(ConfigFileTest, GetValueMissingKeyReturnsNullopt)
{
    EXPECT_FALSE(config.GetValue<bool>("__nonexistent_key__").HasValue());
    EXPECT_FALSE(config.GetValue<i64>("window.nonexistent").HasValue());
}


// ============================================================================
//  SetValue 테스트 (개별 스칼라 설정)
// ============================================================================
TEST_F(ConfigFileTest, SetValueSimple)
{
    ConfigFile new_config;

    EXPECT_TRUE(new_config.SetValue("version", "1.0.0"));
    EXPECT_EQ(new_config.GetValue<String>("version").Value(), "1.0.0");
}

TEST_F(ConfigFileTest, SetValueCreatesIntermediateTables)
{
    ConfigFile new_config;

    EXPECT_TRUE(new_config.SetValue("window.width", 1920));
    EXPECT_TRUE(new_config.SetValue("window.height", 1080));
    EXPECT_TRUE(new_config.SetValue("deep.nested.path.value", 42));

    EXPECT_EQ(new_config.GetValue<i64>("window.width").Value(), 1920);
    EXPECT_EQ(new_config.GetValue<i64>("deep.nested.path.value").Value(), 42);
}

TEST_F(ConfigFileTest, SetValueOverwritesExisting)
{
    ConfigFile new_config;

    EXPECT_TRUE(new_config.SetValue("key", 100));
    EXPECT_EQ(new_config.GetValue<i64>("key").Value(), 100);

    EXPECT_TRUE(new_config.SetValue("key", 200));
    EXPECT_EQ(new_config.GetValue<i64>("key").Value(), 200);
}

TEST_F(ConfigFileTest, SetValueEmptyKeyFails)
{
    ConfigFile new_config;
    EXPECT_FALSE(new_config.SetValue("", 42));
}

TEST_F(ConfigFileTest, SetValueTrailingDotFails)
{
    ConfigFile new_config;
    EXPECT_FALSE(new_config.SetValue("window.", 42));
}


// ============================================================================
//  Save / Load 라운드트립 테스트
// ============================================================================
TEST_F(ConfigFileTest, SaveAndReloadPreservesValues)
{
    ConfigFile new_config;

    WindowSettings window;
    window.width = 2560;
    window.height = 1440;
    window.fullscreen = true;
    window.title = "Round Trip Test";
    window.scale = 1.25f;

    new_config.SetSection(window, "window");

    const auto physical_path = VFS::ToPath(save_test_path);
    struct FileDeleter
    {
        Path path;
        ~FileDeleter() { if (path.Exists()) { fs::Remove(path); } }
    } deleter{ physical_path };

    ASSERT_TRUE(new_config.Save(save_test_path));

    auto reloaded_result = ConfigFile::Load(save_test_path);
    ASSERT_TRUE(reloaded_result.HasValue()) << reloaded_result.Error().CStr();

    auto reloaded_window = reloaded_result.Value().GetSection<WindowSettings>("window");
    EXPECT_EQ(reloaded_window, window);
}

TEST_F(ConfigFileTest, MultiSectionSaveAndReload)
{
    ConfigFile new_config;

    WindowSettings window;
    window.width = 1920;
    window.height = 1080;
    window.title = "Multi Section";

    GraphicsSettings gfx;
    gfx.vsync = true;
    gfx.max_fps = 144;
    gfx.shaders = { "a.vert", "a.frag", "b.vert" };

    LoggingSettings logging;
    logging.level = "warning";
    logging.output_to_file = true;
    logging.log_file_path = "test.log";

    new_config.SetSection(window, "window");
    new_config.SetSection(gfx, "graphics");
    new_config.SetSection(logging, "logging");

    const auto physical_path = VFS::ToPath(save_test_path);
    struct FileDeleter
    {
        Path path;
        ~FileDeleter() { if (path.Exists()) { fs::Remove(path); } }
    } deleter{ physical_path };

    ASSERT_TRUE(new_config.Save(save_test_path));

    auto reloaded = ConfigFile::Load(save_test_path);
    ASSERT_TRUE(reloaded.HasValue());

    auto r_window = reloaded.Value().GetSection<WindowSettings>("window");
    auto r_gfx = reloaded.Value().GetSection<GraphicsSettings>("graphics");
    auto r_logging = reloaded.Value().GetSection<LoggingSettings>("logging");

    EXPECT_EQ(r_window.width, 1920u);
    EXPECT_EQ(r_window.title, "Multi Section");
    EXPECT_EQ(r_gfx.vsync, true);
    EXPECT_EQ(r_gfx.max_fps, 144);
    ASSERT_EQ(r_gfx.shaders.Len(), 3u);
    EXPECT_EQ(r_logging.level, "warning");
    EXPECT_EQ(r_logging.log_file_path, "test.log");
}

TEST_F(ConfigFileTest, GetSectionThenSetSectionFillsMissingValues)
{
    // EditorApplication 패턴 재현:
    // TOML에서 읽고 -> 구조체의 기본값으로 누락분 보충 -> 다시 저장
    ConfigFile new_config;

    // window 섹션에 일부 키만 넣음
    new_config.SetValue("window.width", 1920);
    // height, fullscreen, title, scale 은 TOML에 없음

    auto window = new_config.GetSection<WindowSettings>("window");
    EXPECT_EQ(window.width, 1920u);
    EXPECT_EQ(window.height, 600u);    // 구조체 기본값
    EXPECT_EQ(window.fullscreen, true);  // 구조체 기본값

    // 누락된 값을 채워서 다시 저장
    new_config.SetSection(window, "window");

    // 파일 라운드트립
    const auto physical_path = VFS::ToPath(save_test_path);
    struct FileDeleter
    {
        Path path;
        ~FileDeleter() { if (path.Exists()) { fs::Remove(path); } }
    } deleter{ physical_path };

    ASSERT_TRUE(new_config.Save(save_test_path));

    auto reloaded = ConfigFile::Load(save_test_path);
    ASSERT_TRUE(reloaded.HasValue());

    auto r_window = reloaded.Value().GetSection<WindowSettings>("window");
    EXPECT_EQ(r_window.width, 1920u);    // 원래 값 유지
    EXPECT_EQ(r_window.height, 600u);    // 기본값으로 채워짐
    EXPECT_EQ(r_window.fullscreen, true); // 기본값으로 채워짐
    EXPECT_EQ(r_window.title, "Default Title");
    EXPECT_FLOAT_EQ(r_window.scale, 1.0f);
}


// ============================================================================
//  레거시 형식 호환 테스트
// ============================================================================
TEST_F(ConfigFileTest, GetSectionReadsLegacyFormat)
{
    // 레거시 TomlWriter_v1이 쓴 모양: enum은 정수, f32는 f64로 확장된 값
    const auto physical_path = VFS::ToPath(legacy_toml_path);
    struct FileDeleter
    {
        Path path;
        ~FileDeleter() { if (path.Exists()) { fs::Remove(path); } }
    } deleter{ physical_path };

    ASSERT_TRUE(fs::WriteString(physical_path, "[legacy]\nbusy_wait_ratio = 0.10000000149011612\npresent_mode = 2\n"));

    auto result = ConfigFile::Load(legacy_toml_path);
    ASSERT_TRUE(result.HasValue()) << result.Error().CStr();

    const auto legacy = result.Value().GetSection<LegacyFormatSettings>("legacy");
    EXPECT_EQ(legacy.present_mode, EPresentMode::Immediate);
    EXPECT_EQ(legacy.busy_wait_ratio, 0.1f);
}


// ============================================================================
//  Copy / Move 테스트
// ============================================================================
TEST_F(ConfigFileTest, CopyPreservesData)
{
    ConfigFile copy = config;

    auto original_window = config.GetSection<WindowSettings>("window");
    auto copy_window = copy.GetSection<WindowSettings>("window");

    EXPECT_EQ(original_window, copy_window);
}

TEST_F(ConfigFileTest, MoveTransfersData)
{
    ConfigFile source;
    WindowSettings window;
    window.width = 4096;
    source.SetSection(window, "window");

    ConfigFile moved = std::move(source);

    auto loaded = moved.GetSection<WindowSettings>("window");
    EXPECT_EQ(loaded.width, 4096u);
}
