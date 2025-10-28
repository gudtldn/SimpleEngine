#include "doctest/doctest.h"

#include <filesystem>

#include "SimpleEngine/Utility/Config.h"
#include "SimpleEngine/Utility/PathResolver.h"

#define TOML_EXCEPTIONS 0
#include <SimpleEngine/Core/Container/FixedArray.h>

#include "toml++/toml.h"
#undef TOML_EXCEPTIONS


namespace
{
se::utility::PathResolver& resolver = se::utility::PathResolver::Get();
}

TEST_SUITE("SimpleEngine.Config")
{
using namespace std::string_view_literals;
using namespace std::string_literals;
using namespace se::utility;

[[maybe_unused]]
static struct Init
{
    Init()
    {
        resolver.Mount("Config", std::filesystem::current_path() / "Config");
    }
} _registrar{};

static const VPath test_toml_path = "Config://ConfigTest.toml";
static const VPath non_existent_file_path = "Config://InvalidTest.toml";
static const VPath save_test_toml_path = "Config://SaveTest.toml";


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
            std::ofstream ofs(*resolver.Resolve(non_existent_file_path, false));
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
        std::filesystem::remove(*resolver.Resolve(non_existent_file_path, false)); // 테스트 후 임시 파일 삭제
    }
}

TEST_CASE("get value config file")
{
    const ParseResult v = Config::ReadConfig(test_toml_path);
    CHECK(v.has_value());

    const Config& config = v.value();
    CHECK(config.GetValue<bool>("a_boolean") == true);
    CHECK(config.GetValue<int>("an_integer") == 42);
    CHECK(config.GetValue<float>("a_float") == 3.14159f);
    CHECK(config.GetValue<std::string>("a_string") == "Hello, TOML!");

    CHECK(!config.GetValue<bool>("__MyValue").HasValue());
}

TEST_CASE("get value config file with default value")
{
    ParseResult v = Config::ReadConfig(test_toml_path);
    CHECK(v.has_value());

    Config& config = v.value();
    CHECK(config.GetValueOrStore<bool>("a_boolean", false) == true);
    CHECK(config.GetValueOrStore<int>("an_integer", 100) == 42);
    CHECK(config.GetValueOrStore<float>("a_float", 100.0f) == 3.14159f);
    CHECK(config.GetValueOrStore<std::string>("a_string", "hello world") == "Hello, TOML!");

    CHECK(config.GetValueOrStore<std::string>("MyValue", "TTest") == "TTest");
    CHECK(config.GetValue<std::string>("MyValue") == "TTest");
}

TEST_CASE("get array config file")
{
    const ParseResult v = Config::ReadConfig(test_toml_path);
    CHECK(v.has_value());

    const Config& config = v.value();
    SUBCASE("get int array")
    {
        auto arr = config.GetArray<int>("int_array");
        CHECK(arr.HasValue());
        CHECK(arr->Len() == 5);
        CHECK(arr == se::Array{1, 2, 3, 4, 5});
    }

    SUBCASE("get float array")
    {
        auto arr = config.GetArray<float>("float_array");
        CHECK(arr.HasValue());
        CHECK(arr->Len() == 3);
        CHECK(arr == se::Array{0.5f, 1.5f, 2.5f});
    }

    SUBCASE("get string array")
    {
        auto arr = config.GetArray<std::string>("string_array");
        CHECK(arr.HasValue());
        CHECK(arr->Len() == 3);

        auto check_list = std::array{ "apple", "banana", "cherry" };
        for (usize i = 0; i < arr->Len(); i++)
        {
            CHECK((*arr)[i] == check_list[i]);
        }
    }

    SUBCASE("get bool array")
    {
        auto arr = config.GetArray<bool>("bool_array");
        CHECK(arr.HasValue());
        CHECK(arr->Len() == 4);
        CHECK(arr == se::Array{true, false, true, true});
    }
}

TEST_CASE("get table config file")
{
    const ParseResult v = Config::ReadConfig(test_toml_path);
    CHECK(v.has_value());

    const Config& config = v.value();
    SUBCASE("get table")
    {
        auto window = config.GetTable("window");
        CHECK(window.HasValue());
        CHECK(window->GetValue<int>("width") == 1280);
        CHECK(window->GetValue<int>("height") == 720);
        CHECK(window->GetValue<bool>("fullscreen") == false);
        CHECK(window->GetValue<std::string>("title") == "SimpleEngine Editor");
        CHECK(window->GetValue<float>("scale") == 1.5f);

        auto graphics = config.GetTable("graphics");
        CHECK(graphics.HasValue());
        CHECK(graphics->GetValue<bool>("vsync") == true);
        CHECK(graphics->GetValue<int>("max_fps") == 144);

        auto check_list = se::FixedArray{ "default.vert", "default.frag" };
        auto shaders = graphics->GetArray<std::string>("shaders");
        CHECK(shaders.HasValue());
        CHECK(shaders->Len() == 2);
        for (usize i = 0; i < shaders->Len(); i++)
        {
            CHECK((*shaders)[i] == check_list[i]);
        }

        auto features = graphics->GetTable("features");
        CHECK(features.HasValue());
        CHECK(features->GetValue<std::string>("antialiasing") == "MSAAx4");
        CHECK(features->GetValue<int>("anisotropic_filtering") == 16);
        CHECK(!features->GetValue<std::string>("anisotropic_filtering").HasValue());

        CHECK(graphics->GetArray<int>("multisample_levels") == se::Array{2, 4, 8});
    }
}

TEST_CASE("Config::SetValue and Config::WriteConfig")
{
    struct FileDeleter
    {
        std::filesystem::path path_to_delete;

        FileDeleter(const VPath& p)
            : path_to_delete(*PathResolver::Get().Resolve(p, false))
        {
        }

        ~FileDeleter()
        {
            if (std::filesystem::exists(path_to_delete))
            {
                std::filesystem::remove(path_to_delete);
            }
        }
    };

    Config config;
    config.SetValue("a_boolean", true);
    config.SetValue("an_integer", 42);
    config.SetValue("a_float", 3.14159f);
    config.SetValue("a_string", "Hello, TOML!");

    config.SetValue("int_array", se::Array{ 1, 2, 3, 4, 5 });
    config.SetValue("float_array", se::Array{ 0.5f, 1.5f, 2.5f });
    config.SetValue("string_array", se::Array<se::String>{ "apple", "banana", "cherry" });
    config.SetValue("bool_array", se::Array{ true, false, true, true });

    config.SetValue("window.width", 1280);
    config.SetValue("window.height", 720);
    config.SetValue("window.fullscreen", false);
    config.SetValue("window.title", "SimpleEngine Editor");
    config.SetValue("window.scale", 1.5f);

    config.SetValue("graphics.vsync", true);
    config.SetValue("graphics.max_fps", 144);
    config.SetValue("graphics.shaders", se::Array<se::String>{ "default.vert", "default.frag" });
    config.SetValue("graphics.features.antialiasing", "MSAAx4");
    config.SetValue("graphics.features.anisotropic_filtering", 16);
    config.SetValue("graphics.multisample_levels", se::Array{ 2, 4, 8 });

    FileDeleter file_deleter(save_test_toml_path);
    CHECK(config.WriteConfig(save_test_toml_path));
}
}
