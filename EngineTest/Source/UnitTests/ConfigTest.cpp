#include "../UnitTestEnvironment.h"
#include "gtest/gtest.h"

#include "SimpleEngine/Core/Container/FixedArray.h"
#include "SimpleEngine/Core/Types/VPath.h"
#include "SimpleEngine/Utility/Config.h"
#include "SimpleEngine/Utility/FileSystem.h"

#define TOML_EXCEPTIONS 0
#include "toml++/toml.h"
#undef TOML_EXCEPTIONS


using namespace std::string_view_literals;
using namespace std::string_literals;

using namespace se;
using namespace se::utility;


// --- 테스트 Fixture 정의 ---
class ConfigTest : public ::testing::Test
{
protected:
    virtual void SetUp() override
    {
        ParseResult result = Config::ReadConfig(test_toml_path);
        // 이 Fixture를 사용하는 대부분의 테스트는 이 파일이 성공적으로 로드되는 것을 전제로 합니다.
        // 따라서 EXPECT 대신 ASSERT를 사용하여 실패 시 즉시 중단시킵니다.
        ASSERT_TRUE(result.HasValue()) << "Failed to read base config file for tests: "
                                       << result.Error().description().data();
        config = std::move(result).Value();
    }

    Config config;

    // 테스트 파일 경로들을 멤버 변수로 만들어 접근을 용이하게 합니다.
    static const VPath test_toml_path;
    static const VPath non_existent_file_path;
    static const VPath invalid_toml_path;
    static const VPath save_test_toml_path;
};

// static const 멤버 변수 초기화
const VPath ConfigTest::test_toml_path = "Config://ConfigTest.toml";
const VPath ConfigTest::non_existent_file_path = "Config://NonExistent.toml";
const VPath ConfigTest::invalid_toml_path = "Config://Invalid.toml";
const VPath ConfigTest::save_test_toml_path = "Config://SaveTest.toml";


// --- 파일 읽기 테스트 ---
TEST_F(ConfigTest, ReadExistingAndValidFileSucceeds)
{
    ParseResult result = Config::ReadConfig(test_toml_path);
    EXPECT_TRUE(result.HasValue());
}

TEST_F(ConfigTest, ReadNonExistentFileFails)
{
    ParseResult result = Config::ReadConfig(non_existent_file_path);
    EXPECT_FALSE(result.HasValue());
}

TEST_F(ConfigTest, ReadInvalidTomlFileFails)
{
    // 임시로 유효하지 않은 TOML 파일을 만듭니다.
    const auto physical_path = invalid_toml_path.ToPath();
    FileSystem::WriteString(physical_path, "this = is not valid toml' syntax");

    ParseResult result = Config::ReadConfig(invalid_toml_path);
    EXPECT_FALSE(result.HasValue());

    // 테스트 후 임시 파일 삭제
    FileSystem::Remove(physical_path);
}

// --- 값 가져오기 테스트 ---
TEST_F(ConfigTest, GetValueReturnsCorrectValues)
{
    EXPECT_EQ(config.GetValue<bool>("a_boolean").Value(), true);
    EXPECT_EQ(config.GetValue<int>("an_integer").Value(), 42);
    EXPECT_FLOAT_EQ(config.GetValue<float>("a_float").Value(), 3.14159f);
    EXPECT_EQ(config.GetValue<std::string>("a_string").Value(), "Hello, TOML!");

    // 존재하지 않는 값
    EXPECT_FALSE(config.GetValue<bool>("__MyValue").HasValue());
}

TEST_F(ConfigTest, GetValueOrStoreBehavesCorrectly)
{
    // 이 테스트는 config 객체를 수정하므로, SetUp에서 로드된 공유 객체 대신
    // 자체 복사본을 만들어 사용하는 것이 좋습니다.
    Config local_config = config;

    // 기존 값 확인
    EXPECT_EQ(local_config.GetValueOrStore<bool>("a_boolean", false), true);
    EXPECT_EQ(local_config.GetValueOrStore<int>("an_integer", 100), 42);

    // 새로운 값 저장 및 확인
    EXPECT_EQ(local_config.GetValueOrStore<std::string>("MyValue", "TTest"), "TTest");
    ASSERT_TRUE(local_config.GetValue<std::string>("MyValue").HasValue());
    EXPECT_EQ(local_config.GetValue<std::string>("MyValue").Value(), "TTest");
}


// --- 배열 및 테이블 테스트 ---

TEST_F(ConfigTest, GetArrayForIntegers)
{
    auto arr = config.GetArray<int>("int_array");
    ASSERT_TRUE(arr.HasValue());
    EXPECT_EQ(arr, (se::Array{1, 2, 3, 4, 5}));
}

TEST_F(ConfigTest, GetTableForWindow)
{
    auto window = config.GetTable("window");
    ASSERT_TRUE(window.HasValue());
    EXPECT_EQ(window->GetValue<int>("width").Value(), 1280);
    EXPECT_EQ(window->GetValue<int>("height").Value(), 720);
    EXPECT_EQ(window->GetValue<bool>("fullscreen").Value(), false);
    EXPECT_EQ(window->GetValue<std::string>("title").Value(), "SimpleEngine Editor");
}


// --- 파일 쓰기 테스트 ---

TEST_F(ConfigTest, SetValueAndWriteConfigSavesCorrectly)
{
    Config new_config;
    new_config.SetValue("window.width", 1920);
    new_config.SetValue("window.height", 1080);
    new_config.SetValue("graphics.vsync", false);

    const auto physical_path = save_test_toml_path.ToPath();

    // RAII를 이용한 파일 자동 삭제
    struct FileDeleter
    {
        Path path;
        ~FileDeleter() { if (path.Exists()) FileSystem::Remove(path); }
    } deleter{ physical_path };

    ASSERT_TRUE(new_config.WriteConfig(save_test_toml_path));

    // 저장된 파일을 다시 읽어서 값이 올바른지 검증
    auto reloaded_result = Config::ReadConfig(save_test_toml_path);
    ASSERT_TRUE(reloaded_result.HasValue());

    const Config& reloaded_config = reloaded_result.Value();
    auto window = reloaded_config.GetTable("window");
    ASSERT_TRUE(window.HasValue());
    EXPECT_EQ(window->GetValue<int>("width").Value(), 1920);

    auto graphics = reloaded_config.GetTable("graphics");
    ASSERT_TRUE(graphics.HasValue());
    EXPECT_EQ(graphics->GetValue<bool>("vsync").Value(), false);
}
