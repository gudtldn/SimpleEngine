#include "doctest.h"

import std;
import SE.Core;
import SE.Types;

using namespace se::core;


TEST_SUITE("SimpleEngine.Core.Paths")
{
struct TempDirManager
{
    std::filesystem::path temp_path;

    TempDirManager(const std::filesystem::path& base_name)
    {
        temp_path = std::filesystem::temp_directory_path() / base_name;
        std::filesystem::create_directories(temp_path);
    }

    ~TempDirManager()
    {
        std::filesystem::remove_all(temp_path);
    }
};

TEST_CASE("PathResolver Mount and Resolve")
{
    TempDirManager assets_dir("TestAssets");
    TempDirManager config_dir("TestConfig");

    paths::PathResolver& resolver = paths::PathResolver::Get();
    resolver.Mount(u8"Assets", assets_dir.temp_path);
    resolver.Mount(u8"Config", config_dir.temp_path);

    SUBCASE("Resolve valid paths")
    {
        VPath virtual_path(u8"Assets://textures/player.png");
        Optional<std::filesystem::path> resolved_path = resolver.Resolve(virtual_path);
        CHECK(resolved_path.HasValue());

        std::filesystem::path expected_path = assets_dir.temp_path / "textures/player.png";
        CHECK(resolved_path.Value().generic_string() == expected_path.generic_string());
    }

    SUBCASE("Resolve path with unmounted scheme")
    {
        VPath virtual_path(u8"InvalidScheme://some/path.txt");
        Optional<std::filesystem::path> resolved_path = resolver.Resolve(virtual_path);
        CHECK_FALSE(resolved_path.HasValue());
    }

    SUBCASE("Resolve path with no scheme")
    {
        VPath virtual_path(u8"some/relative/path.txt");
        Optional<std::filesystem::path> resolved_path = resolver.Resolve(virtual_path);
        CHECK_FALSE(resolved_path.HasValue());
    }

    resolver.Unmount(u8"Assets");
    resolver.Unmount(u8"Config");
}

TEST_CASE("PathResolver Unresolve")
{
    TempDirManager assets_dir("TestAssets");
    TempDirManager mods_dir("TestMods");

    paths::PathResolver& resolver = paths::PathResolver::Get();
    resolver.Mount(u8"Assets", assets_dir.temp_path);
    resolver.Mount(u8"Mods", mods_dir.temp_path);

    SUBCASE("Unresolve a path within a mount point")
    {
        std::filesystem::path physical_path = assets_dir.temp_path / "scripts/main.lua";
        Optional<VPath> virtual_path = resolver.Unresolve(physical_path);
        CHECK(virtual_path.HasValue());
        CHECK(virtual_path.Value().ToU8String() == u8"Assets://scripts/main.lua");
    }

    SUBCASE("Unresolve a path outside any mount point")
    {
        std::filesystem::path physical_path = std::filesystem::temp_directory_path() / "unrelated_file.txt";
        Optional<VPath> virtual_path = resolver.Unresolve(physical_path);
        CHECK_FALSE(virtual_path.HasValue());
    }

    resolver.Unmount(u8"Assets");
    resolver.Unmount(u8"Mods");
}

TEST_CASE("PathResolver Priority and Unresolve")
{
    TempDirManager base_game_dir("BaseGame");
    TempDirManager mod_override_dir("ModOverride");

    paths::PathResolver& resolver = paths::PathResolver::Get();

    // Base game assets have lower priority
    resolver.Mount(u8"Game", base_game_dir.temp_path, 0);
    // Mod assets have higher priority
    resolver.Mount(u8"Game", mod_override_dir.temp_path, 10);

    SUBCASE("Unresolve prefers higher priority mount")
    {
        std::filesystem::path modded_file = mod_override_dir.temp_path / "characters/hero.asset";
        Optional<VPath> virtual_path = resolver.Unresolve(modded_file);
        CHECK(virtual_path.HasValue());
        // Even though it also matches the BaseGame mount, it should choose the higher priority one.
        CHECK(virtual_path.Value().ToU8String() == u8"Game://characters/hero.asset");
    }

    SUBCASE("Resolve uses the first registered mount (highest priority)")
    {
        VPath virtual_path(u8"Game://config/settings.ini");
        Optional<std::filesystem::path> resolved_path = resolver.Resolve(virtual_path);
        CHECK(resolved_path.HasValue());

        // Should resolve to the higher priority directory
        std::filesystem::path expected_path = mod_override_dir.temp_path / "config/settings.ini";
        CHECK(resolved_path.Value().generic_string() == expected_path.generic_string());
    }

    resolver.Unmount(u8"Game");
}
}
