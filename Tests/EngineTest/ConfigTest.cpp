#include "doctest.h"
#include "../../../SimpleEngine/ThirdParty/tomlplusplus/toml.hpp"

import std;
import SimpleEngine.Config;


TEST_SUITE("SimpleEngine.Config")
{
static const std::filesystem::path solution_path = std::filesystem::current_path().parent_path().parent_path();
static const std::filesystem::path test_toml_path = solution_path / u8"Config/ConfigTest.toml";
static const std::filesystem::path non_existent_file_path = solution_path / u8"Config" / u8"InvalidTest.toml";

using namespace std::string_view_literals;
using namespace std::string_literals;
using namespace se::config;


// --- 파일 읽기 테스트 ---
TEST_CASE("Config::ReadConfig - File Handling")
{
    SUBCASE("Reading an existing and valid config file")
    {
        const ParseResult result = Config::ReadConfig(test_toml_path);
        CHECK(result.has_value());
        if (!result.has_value())
        {
            FAIL_CHECK(
                "Failed to read config file: " << result.error().description() <<
                " at line " << result.error().source().begin.line <<
                ", column " << result.error().source().begin.column
            );
        }
    }

    SUBCASE("Attempting to read a non-existent config file")
    {
        const ParseResult result = Config::ReadConfig(non_existent_file_path);
        CHECK_FALSE(result.has_value());
        if (result.has_value())
        {
            FAIL_CHECK("ReadConfig succeeded for a non-existent file.");
        }
        else
        {
            // toml++는 파일 열기 실패 시 특정 에러를 반환할 수 있습니다.
            // (예: toml::parse_error의 특정 메시지 또는 타입)
            // 여기서는 단순히 실패했는지 여부만 확인합니다.
            MESSAGE("Successfully failed to read non-existent file as expected.");
        }
    }

    SUBCASE("Attempting to read an invalid TOML file")
    {
        // 임시로 유효하지 않은 TOML 파일을 만듭니다.
        {
            std::ofstream ofs(non_existent_file_path);
            ofs << "this = is not valid toml syntax because of this character '";
        }
        const ParseResult result = Config::ReadConfig(non_existent_file_path);
        CHECK_FALSE(result.has_value());
        if (result.has_value())
        {
            FAIL_CHECK("ReadConfig succeeded for an invalid TOML file.");
        }
        else
        {
            MESSAGE("Successfully failed to read invalid TOML file as expected: " << result.error().description().data());
        }
        std::filesystem::remove(non_existent_file_path); // 테스트 후 임시 파일 삭제
    }
}

TEST_CASE("get value config file")
{
    const ParseResult v = Config::ReadConfig(test_toml_path);
    CHECK(v.has_value());

    const Config& config = v.value();
    CHECK(config.GetValue<bool>(u8"a_boolean") == true);
    CHECK(config.GetValue<int>(u8"an_integer") == 42);
    CHECK(config.GetValue<float>(u8"a_float") == 3.14159f);
    CHECK(config.GetValue<std::u8string>(u8"a_string") == u8"Hello, TOML!");

    CHECK(!config.GetValue<bool>(u8"__MyValue").has_value());
}

TEST_CASE("get value config file with default value")
{
    const ParseResult v = Config::ReadConfig(test_toml_path);
    CHECK(v.has_value());

    const Config& config = v.value();
    CHECK(config.GetValueOr<bool>(u8"a_boolean", false) == true);
    CHECK(config.GetValueOr<int>(u8"an_integer", 100) == 42);
    CHECK(config.GetValueOr<float>(u8"a_float", 100.0f) == 3.14159f);
    CHECK(config.GetValueOr<std::u8string>(u8"a_string", u8"hello world") == u8"Hello, TOML!");

    CHECK(config.GetValueOr<bool>(u8"MyValue", true) == true);
}

TEST_CASE("get array config file")
{
    const ParseResult v = Config::ReadConfig(test_toml_path);
    CHECK(v.has_value());

    const Config& config = v.value();
    SUBCASE("get int array")
    {
        auto arr = config.GetArray<int>(u8"int_array");
        CHECK(arr.has_value());
        CHECK(arr->size() == 5);
        CHECK(arr == std::vector{1, 2, 3, 4, 5});
    }

    SUBCASE("get float array")
    {
        auto arr = config.GetArray<float>(u8"float_array");
        CHECK(arr.has_value());
        CHECK(arr->size() == 3);
        CHECK(arr == std::vector{0.5f, 1.5f, 2.5f});
    }

    SUBCASE("get string array")
    {
        auto arr = config.GetArray<std::u8string>(u8"string_array");
        CHECK(arr.has_value());
        CHECK(arr->size() == 3);

        auto check_list = std::array{u8"apple", u8"banana", u8"cherry"};
        for (size_t i = 0; i < arr->size(); i++)
        {
            CHECK((*arr)[i] == check_list[i]);
        }
    }

    SUBCASE("get bool array")
    {
        auto arr = config.GetArray<bool>(u8"bool_array");
        CHECK(arr.has_value());
        CHECK(arr->size() == 4);
        CHECK(arr == std::vector{true, false, true, true});
    }
}

TEST_CASE("get table config file")
{
    const ParseResult v = Config::ReadConfig(test_toml_path);
    CHECK(v.has_value());

    const Config& config = v.value();
    SUBCASE("get table")
    {
        auto window = config.GetTable(u8"window");
        CHECK(window.has_value());
        CHECK(window->GetValue<int>(u8"width") == 1280);
        CHECK(window->GetValue<int>(u8"height") == 720);
        CHECK(window->GetValue<bool>(u8"fullscreen") == false);
        CHECK(window->GetValue<std::u8string>(u8"title") == u8"SimpleEngine Editor");
        CHECK(window->GetValue<float>(u8"scale") == 1.5f);

        auto graphics = config.GetTable(u8"graphics");
        CHECK(graphics.has_value());
        CHECK(graphics->GetValue<bool>(u8"vsync") == true);
        CHECK(graphics->GetValue<int>(u8"max_fps") == 144);

        auto check_list = std::array{u8"default.vert", u8"default.frag"};
        auto shaders = graphics->GetArray<std::u8string>(u8"shaders");
        CHECK(shaders.has_value());
        CHECK(shaders->size() == 2);
        for (size_t i = 0; i < shaders->size(); i++)
        {
            CHECK((*shaders)[i] == check_list[i]);
        }

        auto features = graphics->GetTable(u8"features");
        CHECK(features.has_value());
        CHECK(features->GetValue<std::u8string>(u8"antialiasing") == u8"MSAAx4");
        CHECK(features->GetValue<int>(u8"anisotropic_filtering") == 16);
        CHECK(!features->GetValue<std::u8string>(u8"anisotropic_filtering").has_value());

        CHECK(graphics->GetArray<int>(u8"multisample_levels") == std::vector{2, 4, 8});
    }
}
}
