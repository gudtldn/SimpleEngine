#include "gtest/gtest.h"

#include <filesystem>
#include <fstream>

#include "SimpleEngine/Core/Container/HashSet.h"
#include "SimpleEngine/Core/Container/Optional.h"
#include "SimpleEngine/Core/Types/StringName.h"
#include "SimpleEngine/Core/Types/VPath.h"
#include "SimpleEngine/Utility/PathResolver.h"

using namespace se;
using namespace se::utility;


namespace
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
    HashSet<StringName> mounted_schemes;

    PathResolverGuard(PathResolver& res)
        : resolver(res)
    {
    }

    void Mount(const StringName& scheme, const std::filesystem::path& physical_path, int32_t priority = 0)
    {
        resolver.Mount(scheme, physical_path, priority);
        mounted_schemes.Add(scheme);
    }

    ~PathResolverGuard()
    {
        for (const auto& scheme : mounted_schemes)
        {
            resolver.Unmount(scheme);
        }
    }
};
}

// --- PathResolver 테스트를 위한 Fixture 클래스 ---
class PathResolverTest : public ::testing::Test
{
protected:
    // 각 테스트는 완전히 독립된 환경에서 실행되도록
    // Fixture 내에서 TempDirManager와 PathResolverGuard를 관리합니다.
    TempDirManager assets_dir{"TestAssets"};
    PathResolverGuard guard{PathResolver::Get()};

    // SetUp()에서 공통적인 Mount 작업을 수행할 수 있습니다.
    virtual void SetUp() override
    {
        guard.Mount("Assets", assets_dir.temp_path);
    }
};

// --- Mount 및 Resolve 테스트 ---

TEST_F(PathResolverTest, ResolveFailsIfFileDoesNotExist)
{
    VPath virtual_path("Assets://textures/non_existent.png");
    Optional<std::filesystem::path> resolved_path = guard.resolver.Resolve(virtual_path);
    EXPECT_FALSE(resolved_path.HasValue());
}

TEST_F(PathResolverTest, ResolveSucceedsIfFileExists)
{
    assets_dir.CreateDummyFile("textures/player.png");

    VPath virtual_path("Assets://textures/player.png");
    Optional<std::filesystem::path> resolved_path = guard.resolver.Resolve(virtual_path);

    ASSERT_TRUE(resolved_path.HasValue());

    std::filesystem::path expected_path = std::filesystem::absolute(assets_dir.temp_path / "textures/player.png");
    EXPECT_EQ(resolved_path.Value(), expected_path);
}

TEST_F(PathResolverTest, ResolveFailsForUnmountedScheme)
{
    VPath virtual_path("InvalidScheme://some/path.txt");
    Optional<std::filesystem::path> resolved_path = guard.resolver.Resolve(virtual_path);
    EXPECT_FALSE(resolved_path.HasValue());
}

TEST_F(PathResolverTest, ResolveFailsForPathWithNoScheme)
{
    VPath virtual_path("some/relative/path.txt");
    Optional<std::filesystem::path> resolved_path = guard.resolver.Resolve(virtual_path);
    EXPECT_FALSE(resolved_path.HasValue());
}

// --- Unresolve 테스트 ---

TEST_F(PathResolverTest, UnresolveSucceedsForPathWithinMountPoint)
{
    std::filesystem::path physical_path = assets_dir.temp_path / "scripts/main.lua";
    Optional<VPath> virtual_path = guard.resolver.Unresolve(physical_path);

    ASSERT_TRUE(virtual_path.HasValue());
    EXPECT_EQ(virtual_path.Value().ToString(), "Assets://scripts/main.lua");
}

TEST_F(PathResolverTest, UnresolveFailsForPathOutsideMountPoint)
{
    std::filesystem::path physical_path = std::filesystem::temp_directory_path() / "unrelated_file.txt";
    Optional<VPath> virtual_path = guard.resolver.Unresolve(physical_path);
    EXPECT_FALSE(virtual_path.HasValue());
}


// --- 우선순위(Priority) 테스트를 위한 별도 Fixture ---
// 다른 테스트와 설정(Mount)이 다르므로 별도의 Fixture로 분리하는 것이 깔끔합니다.
class PathResolverPriorityTest : public ::testing::Test
{
protected:
    TempDirManager base_game_dir{"BaseGame"};
    TempDirManager mod_override_dir{"ModOverride"};
    PathResolverGuard guard{PathResolver::Get()};

    virtual void SetUp() override
    {
        // Base game assets (낮은 우선순위)
        guard.Mount("Game", base_game_dir.temp_path, 0);
        // Mod assets (높은 우선순위)
        guard.Mount("Game", mod_override_dir.temp_path, 10);
    }

    const VPath virtual_path{"Game://config/settings.ini"};
};

TEST_F(PathResolverPriorityTest, ResolveUsesHigherPriorityPathIfExists)
{
    mod_override_dir.CreateDummyFile("config/settings.ini");
    base_game_dir.CreateDummyFile("config/settings.ini");

    Optional<std::filesystem::path> resolved_path = guard.resolver.Resolve(virtual_path);

    ASSERT_TRUE(resolved_path.HasValue());
    std::filesystem::path expected_path = std::filesystem::absolute(mod_override_dir.temp_path / "config/settings.ini");
    EXPECT_EQ(resolved_path.Value(), expected_path);
}

TEST_F(PathResolverPriorityTest, ResolveFallsBackToLowerPriorityPath)
{
    base_game_dir.CreateDummyFile("config/settings.ini");

    Optional<std::filesystem::path> resolved_path = guard.resolver.Resolve(virtual_path);

    ASSERT_TRUE(resolved_path.HasValue());
    std::filesystem::path expected_path = std::filesystem::absolute(base_game_dir.temp_path / "config/settings.ini");
    EXPECT_EQ(resolved_path.Value(), expected_path);
}

TEST_F(PathResolverPriorityTest, ResolveFailsIfFileExistsInNeither)
{
    Optional<std::filesystem::path> resolved_path = guard.resolver.Resolve(virtual_path);
    EXPECT_FALSE(resolved_path.HasValue());
}


// --- Unresolve 우선순위 테스트를 위한 Fixture ---
class PathResolverUnresolvePriorityTest : public ::testing::Test
{
protected:
    TempDirManager common_dir{"CommonDir"};
    PathResolverGuard guard{PathResolver::Get()};
};

TEST_F(PathResolverUnresolvePriorityTest, UnresolvePrefersLongestPathMatch)
{
    guard.Mount("Generic", common_dir.temp_path, 10);
    guard.Mount("Specific", common_dir.temp_path / "specific", 0);

    std::filesystem::path physical_path = common_dir.temp_path / "specific" / "file.txt";
    Optional<VPath> virtual_path = guard.resolver.Unresolve(physical_path);

    ASSERT_TRUE(virtual_path.HasValue());
    // 더 긴 경로인 "Specific"을 선택해야 함
    EXPECT_EQ(virtual_path.Value().ToString(), "Specific://file.txt");
}

TEST_F(PathResolverUnresolvePriorityTest, UnresolvePrefersHigherPriorityForSameLengthPaths)
{
    guard.Mount("Base", common_dir.temp_path, 0);
    guard.Mount("Mod", common_dir.temp_path, 10);

    std::filesystem::path physical_path = common_dir.temp_path / "file.txt";
    Optional<VPath> virtual_path = guard.resolver.Unresolve(physical_path);

    ASSERT_TRUE(virtual_path.HasValue());
    // 경로 길이가 같으므로 우선순위가 높은 "Mod"를 선택해야 함
    EXPECT_EQ(virtual_path.Value().ToString(), "Mod://file.txt");
}
