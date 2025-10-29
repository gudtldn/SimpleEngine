#include "doctest/doctest.h"

#include <filesystem>
#include <fstream> // For creating dummy files

#include "SimpleEngine/Core/Container/Optional.h"
#include "SimpleEngine/Core/Types/StringName.h" // For StringName
#include "SimpleEngine/Core/Types/VPath.h"
#include "SimpleEngine/Utility/PathResolver.h"


using namespace se;
using namespace se::utility;


TEST_SUITE("SimpleEngine.Utility.PathResolver")
{
struct TempDirManager
{
    std::filesystem::path temp_path;

    TempDirManager(const std::filesystem::path& base_name)
    {
        temp_path = std::filesystem::temp_directory_path() / "SimpleEngineTest" / base_name;
        std::filesystem::create_directories(temp_path);
    }

    ~TempDirManager()
    {
        std::error_code ec;
        std::filesystem::remove_all(temp_path, ec);
        // Ignore errors, as cleanup might fail on some platforms
    }

    void CreateDummyFile(const std::filesystem::path& relative_path) const
    {
        const auto full_path = temp_path / relative_path;
        std::filesystem::create_directories(full_path.parent_path());
        std::ofstream ofs(full_path);
        ofs << "dummy content";
    }
};

// Helper to ensure resolver is clean before and after tests
struct PathResolverGuard
{
    PathResolver& resolver;
    std::vector<StringName> mounted_schemes;

    PathResolverGuard(PathResolver& res)
        : resolver(res)
    {
    }

    void Mount(const StringName& scheme, const std::filesystem::path& physical_path, int32_t priority = 0)
    {
        resolver.Mount(scheme, physical_path, priority);
        // Avoid duplicates
        if (std::find(mounted_schemes.begin(), mounted_schemes.end(), scheme) == mounted_schemes.end())
        {
            mounted_schemes.push_back(scheme);
        }
    }

    ~PathResolverGuard()
    {
        for (const auto& scheme : mounted_schemes)
        {
            resolver.Unmount(scheme);
        }
    }
};


TEST_CASE("PathResolver Mount and Resolve")
{
    TempDirManager assets_dir("TestAssets");
    PathResolverGuard guard(PathResolver::Get());
    guard.Mount("Assets", assets_dir.temp_path);

    SUBCASE("Resolve fails if file does not exist")
    {
        VPath virtual_path("Assets://textures/non_existent.png");
        Optional<std::filesystem::path> resolved_path = guard.resolver.Resolve(virtual_path);
        CHECK_FALSE(resolved_path.HasValue());
    }

    SUBCASE("Resolve succeeds if file exists")
    {
        assets_dir.CreateDummyFile("textures/player.png");

        VPath virtual_path("Assets://textures/player.png");
        Optional<std::filesystem::path> resolved_path = guard.resolver.Resolve(virtual_path);
        CHECK(resolved_path.HasValue());

        std::filesystem::path expected_path = std::filesystem::absolute(assets_dir.temp_path / "textures/player.png");
        CHECK(resolved_path.Value() == expected_path);
    }

    SUBCASE("Resolve path with unmounted scheme")
    {
        VPath virtual_path("InvalidScheme://some/path.txt");
        Optional<std::filesystem::path> resolved_path = guard.resolver.Resolve(virtual_path);
        CHECK_FALSE(resolved_path.HasValue());
    }

    SUBCASE("Resolve path with no scheme")
    {
        VPath virtual_path("some/relative/path.txt");
        Optional<std::filesystem::path> resolved_path = guard.resolver.Resolve(virtual_path);
        CHECK_FALSE(resolved_path.HasValue());
    }
}

TEST_CASE("PathResolver Unresolve")
{
    TempDirManager assets_dir("TestAssets");
    PathResolverGuard guard(PathResolver::Get());
    guard.Mount("Assets", assets_dir.temp_path);

    SUBCASE("Unresolve a path within a mount point")
    {
        std::filesystem::path physical_path = assets_dir.temp_path / "scripts/main.lua";
        Optional<VPath> virtual_path = guard.resolver.Unresolve(physical_path);
        CHECK(virtual_path.HasValue());
        CHECK(virtual_path.Value().ToString() == "Assets://scripts/main.lua");
    }

    SUBCASE("Unresolve a path outside any mount point")
    {
        std::filesystem::path physical_path = std::filesystem::temp_directory_path() / "unrelated_file.txt";
        Optional<VPath> virtual_path = guard.resolver.Unresolve(physical_path);
        CHECK_FALSE(virtual_path.HasValue());
    }
}

TEST_CASE("PathResolver Priority and Fallback on Resolve")
{
    TempDirManager base_game_dir("BaseGame");
    TempDirManager mod_override_dir("ModOverride");
    PathResolverGuard guard(PathResolver::Get());

    // Base game assets have lower priority
    guard.Mount("Game", base_game_dir.temp_path, 0);
    // Mod assets have higher priority
    guard.Mount("Game", mod_override_dir.temp_path, 10);

    VPath virtual_path("Game://config/settings.ini");

    SUBCASE("Resolve uses higher priority path if file exists there")
    {
        mod_override_dir.CreateDummyFile("config/settings.ini");
        base_game_dir.CreateDummyFile("config/settings.ini"); // Also in base

        Optional<std::filesystem::path> resolved_path = guard.resolver.Resolve(virtual_path);
        CHECK(resolved_path.HasValue());

        std::filesystem::path expected_path = std::filesystem::absolute(mod_override_dir.temp_path / "config/settings.ini");
        CHECK(resolved_path.Value() == expected_path);
    }

    SUBCASE("Resolve falls back to lower priority path if file only exists there")
    {
        base_game_dir.CreateDummyFile("config/settings.ini");

        Optional<std::filesystem::path> resolved_path = guard.resolver.Resolve(virtual_path);
        CHECK(resolved_path.HasValue());

        std::filesystem::path expected_path = std::filesystem::absolute(base_game_dir.temp_path / "config/settings.ini");
        CHECK(resolved_path.Value() == expected_path);
    }

    SUBCASE("Resolve fails if file exists in neither")
    {
        Optional<std::filesystem::path> resolved_path = guard.resolver.Resolve(virtual_path);
        CHECK_FALSE(resolved_path.HasValue());
    }
}

TEST_CASE("PathResolver Priority on Unresolve")
{
    TempDirManager common_dir("CommonDir");
    TempDirManager specific_dir("SpecificDir");
    PathResolverGuard guard(PathResolver::Get());

    SUBCASE("Unresolve prefers longest path match over priority")
    {
        guard.Mount("Generic", common_dir.temp_path, 10);              // High priority
        guard.Mount("Specific", common_dir.temp_path / "specific", 0); // Low priority

        std::filesystem::path physical_path = common_dir.temp_path / "specific" / "file.txt";
        Optional<VPath> virtual_path = guard.resolver.Unresolve(physical_path);

        CHECK(virtual_path.HasValue());
        // Should choose "Specific" because it's a longer, more specific match, despite lower priority
        CHECK(virtual_path.Value().ToString() == "Specific://file.txt");
    }

    SUBCASE("Unresolve prefers higher priority for same-length paths")
    {
        guard.Mount("Base", common_dir.temp_path, 0); // Low priority
        guard.Mount("Mod", common_dir.temp_path, 10); // High priority

        std::filesystem::path physical_path = common_dir.temp_path / "file.txt";
        Optional<VPath> virtual_path = guard.resolver.Unresolve(physical_path);

        CHECK(virtual_path.HasValue());
        // Paths are same length, so higher priority "Mod" should be chosen
        CHECK(virtual_path.Value().ToString() == "Mod://file.txt");
    }
}
}
