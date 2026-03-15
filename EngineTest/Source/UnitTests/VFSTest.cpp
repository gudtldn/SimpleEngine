#include "gtest/gtest.h"

#include "SimpleEngine/Core/Container/HashSet.h"
#include "SimpleEngine/Core/Container/Optional.h"
#include "SimpleEngine/Core/Container/StringView.h"
#include "SimpleEngine/Core/FileSystem/FileSystem.h"
#include "SimpleEngine/Core/FileSystem/VFS.h"
#include "SimpleEngine/Core/Types/VPath.h"

#include "SDL3/SDL_filesystem.h"

using namespace se;


namespace
{
struct TempDirManager
{
    Path temp_path;

    TempDirManager(StringView base_name)
    {
        char* pref = SDL_GetPrefPath("SimpleEngine", "Tests");
        temp_path = Path(pref) / Path(base_name);
        SDL_free(pref);
        FileSystem::CreateDirectories(temp_path);
    }

    ~TempDirManager()
    {
        FileSystem::RemoveAll(temp_path);
    }

    void CreateDummyFile(StringView relative_path) const
    {
        const Path full_path = temp_path / Path(relative_path);
        if (const auto parent = full_path.Parent())
        {
            FileSystem::CreateDirectories(*parent);
        }
        FileSystem::WriteString(full_path, "dummy content");
    }
};

// Helper to ensure VFS is clean before and after tests
struct VFSGuard
{
    VFS& vfs;
    HashSet<String> mounted_schemes;

    VFSGuard(VFS& v)
        : vfs(v)
    {
    }

    void Mount(StringView scheme, const Path& physical_path, int32_t priority = 0)
    {
        vfs.Mount(scheme, physical_path, priority);
        mounted_schemes.Insert(String(scheme));
    }

    ~VFSGuard()
    {
        for (const auto& scheme : mounted_schemes)
        {
            vfs.Unmount(scheme.Bytes());
        }
    }
};
}

// --- VFS 테스트를 위한 Fixture 클래스 ---
class VFSTest : public ::testing::Test
{
protected:
    // 각 테스트는 완전히 독립된 환경에서 실행되도록
    // Fixture 내에서 TempDirManager와 VFSGuard를 관리합니다.
    TempDirManager assets_dir{"TestAssets"};
    VFSGuard guard{VFS::Get()};

    // SetUp()에서 공통적인 Mount 작업을 수행할 수 있습니다.
    virtual void SetUp() override
    {
        guard.Mount("Assets", assets_dir.temp_path);
    }
};

// --- Mount 및 Resolve 테스트 ---

TEST_F(VFSTest, ResolveFailsIfFileDoesNotExist)
{
    VPath virtual_path("Assets://textures/non_existent.png");
    Optional<Path> resolved_path = VFS::Resolve(virtual_path);
    EXPECT_FALSE(resolved_path.HasValue());
}

TEST_F(VFSTest, ResolveSucceedsIfFileExists)
{
    assets_dir.CreateDummyFile("textures/player.png");

    VPath virtual_path("Assets://textures/player.png");
    Optional<Path> resolved_path = VFS::Resolve(virtual_path);

    ASSERT_TRUE(resolved_path.HasValue());

    Path expected_path = FileSystem::Absolute(assets_dir.temp_path / Path("textures/player.png"));
    EXPECT_EQ(resolved_path, expected_path);
}

TEST_F(VFSTest, ResolveFailsForUnmountedScheme)
{
    VPath virtual_path("InvalidScheme://some/path.txt");
    Optional<Path> resolved_path = VFS::Resolve(virtual_path);
    EXPECT_FALSE(resolved_path.HasValue());
}

TEST_F(VFSTest, ResolveFailsForPathWithNoScheme)
{
    VPath virtual_path("some/relative/path.txt");
    Optional<Path> resolved_path = VFS::Resolve(virtual_path);
    EXPECT_FALSE(resolved_path.HasValue());
}

// --- Unresolve 테스트 ---

TEST_F(VFSTest, UnresolveSucceedsForPathWithinMountPoint)
{
    Path physical_path = assets_dir.temp_path / Path("scripts/main.lua");
    Optional<VPath> virtual_path = VFS::Unresolve(physical_path);

    ASSERT_TRUE(virtual_path.HasValue());
    EXPECT_EQ(virtual_path.Value().ToString(), "Assets://scripts/main.lua");
}

TEST_F(VFSTest, UnresolveFailsForPathOutsideMountPoint)
{
    char* pref = SDL_GetPrefPath("SimpleEngine", "Tests");
    Path physical_path = Path(pref) / Path("unrelated_file.txt");
    SDL_free(pref);
    Optional<VPath> virtual_path = VFS::Unresolve(physical_path);
    EXPECT_FALSE(virtual_path.HasValue());
}


// --- 우선순위(Priority) 테스트를 위한 별도 Fixture ---
// 다른 테스트와 설정(Mount)이 다르므로 별도의 Fixture로 분리하는 것이 깔끔합니다.
class VFSPriorityTest : public ::testing::Test
{
protected:
    TempDirManager base_game_dir{"BaseGame"};
    TempDirManager mod_override_dir{"ModOverride"};
    VFSGuard guard{VFS::Get()};

    virtual void SetUp() override
    {
        // Base game assets (낮은 우선순위)
        guard.Mount("Game", base_game_dir.temp_path, 0);
        // Mod assets (높은 우선순위)
        guard.Mount("Game", mod_override_dir.temp_path, 10);
    }

    const VPath virtual_path{"Game://config/settings.ini"};
};

TEST_F(VFSPriorityTest, ResolveUsesHigherPriorityPathIfExists)
{
    mod_override_dir.CreateDummyFile("config/settings.ini");
    base_game_dir.CreateDummyFile("config/settings.ini");

    Optional<Path> resolved_path = VFS::Resolve(virtual_path);

    ASSERT_TRUE(resolved_path.HasValue());
    Path expected_path = FileSystem::Absolute(mod_override_dir.temp_path / Path("config/settings.ini"));
    EXPECT_EQ(resolved_path.Value(), expected_path);
}

TEST_F(VFSPriorityTest, ResolveFallsBackToLowerPriorityPath)
{
    base_game_dir.CreateDummyFile("config/settings.ini");

    Optional<Path> resolved_path = VFS::Resolve(virtual_path);

    ASSERT_TRUE(resolved_path.HasValue());
    Path expected_path = FileSystem::Absolute(base_game_dir.temp_path / Path("config/settings.ini"));
    EXPECT_EQ(resolved_path.Value(), expected_path);
}

TEST_F(VFSPriorityTest, ResolveFailsIfFileExistsInNeither)
{
    Optional<Path> resolved_path = VFS::Resolve(virtual_path);
    EXPECT_FALSE(resolved_path.HasValue());
}


// --- Unresolve 우선순위 테스트를 위한 Fixture ---
class VFSUnresolvePriorityTest : public ::testing::Test
{
protected:
    TempDirManager common_dir{"CommonDir"};
    VFSGuard guard{VFS::Get()};
};

TEST_F(VFSUnresolvePriorityTest, UnresolvePrefersLongestPathMatch)
{
    guard.Mount("Generic", common_dir.temp_path, 10);
    guard.Mount("Specific", common_dir.temp_path / Path("specific"), 0);

    Path physical_path = common_dir.temp_path / Path("specific") / Path("file.txt");
    Optional<VPath> virtual_path = VFS::Unresolve(physical_path);

    ASSERT_TRUE(virtual_path.HasValue());
    // 더 긴 경로인 "Specific"을 선택해야 함
    EXPECT_EQ(virtual_path.Value().ToString(), "Specific://file.txt");
}

TEST_F(VFSUnresolvePriorityTest, UnresolvePrefersHigherPriorityForSameLengthPaths)
{
    guard.Mount("Base", common_dir.temp_path, 0);
    guard.Mount("Mod", common_dir.temp_path, 10);

    Path physical_path = common_dir.temp_path / Path("file.txt");
    Optional<VPath> virtual_path = VFS::Unresolve(physical_path);

    ASSERT_TRUE(virtual_path.HasValue());
    // 경로 길이가 같으므로 우선순위가 높은 "Mod"를 선택해야 함
    EXPECT_EQ(virtual_path.Value().ToString(), "Mod://file.txt");
}


// =============================================================================
// Path Traversal 보안 테스트 (RISK-3: Path Traversal via "..")
// =============================================================================

class VFSPathTraversalTest : public ::testing::Test
{
protected:
    TempDirManager mount_dir{"TraversalMount"};
    TempDirManager parent_dir{"TraversalParent"};
    VFSGuard guard{VFS::Get()};

    void SetUp() override
    {
        guard.Mount("Sandbox", mount_dir.temp_path);
        // 마운트 포인트 외부에 파일 생성
        parent_dir.CreateDummyFile("secret.txt");
    }
};

TEST_F(VFSPathTraversalTest, DotDotDoesNotEscapeMountPoint)
{
    // RISK-3: "../../../" 패턴으로 마운트 포인트 탈출 시도
    // Path::operator/가 ".."을 해소하므로 결과가 마운트 포인트 내부로 한정됨
    // 하지만 해소 결과가 실제로 마운트 포인트 내부인지 확인하는 containment check는 없음
    VPath traversal_path("Sandbox://../../secret.txt");

    // Resolve는 실제 파일이 존재하는지 확인하므로 여기서는 파일이 없으면 NullOpt
    Optional<Path> resolved = VFS::Resolve(traversal_path);

    if (resolved.HasValue())
    {
        // resolve가 성공했다면, 결과가 마운트 포인트 내부인지 확인
        EXPECT_TRUE(resolved->IsSubPathOf(mount_dir.temp_path))
            << "Path traversal escaped mount point! Resolved to: " << resolved->ToString().CStr();
    }
}

TEST_F(VFSPathTraversalTest, ToPathWithDotDot)
{
    // ToPath는 존재 여부를 확인하지 않으므로 항상 경로를 반환
    VPath traversal_path("Sandbox://../../escape");
    Path result = VFS::ToPath(traversal_path);

    if (!result.IsEmpty())
    {
        // 결과 경로가 마운트 포인트를 벗어나지 않아야 함
        EXPECT_TRUE(result.IsSubPathOf(mount_dir.temp_path))
            << "ToPath escaped mount point! Result: " << result.ToString().CStr();
    }
}


// =============================================================================
// VFS 마운트 엣지 케이스 (Mount Edge Cases)
// =============================================================================

class VFSMountEdgeCaseTest : public ::testing::Test
{
protected:
    TempDirManager temp_dir{"MountEdge"};
    VFSGuard guard{VFS::Get()};
};

TEST_F(VFSMountEdgeCaseTest, MountSameSchemeAndPathTwiceIsNoop)
{
    guard.Mount("Test", temp_dir.temp_path, 0);
    guard.Mount("Test", temp_dir.temp_path, 0);  // 중복 마운트

    // 파일 생성 후 resolve
    temp_dir.CreateDummyFile("file.txt");
    Optional<Path> resolved = VFS::Resolve(VPath("Test://file.txt"));
    EXPECT_TRUE(resolved.HasValue());
}

TEST_F(VFSMountEdgeCaseTest, UnmountNonExistentSchemeDoesNotCrash)
{
    // 등록되지 않은 스킴 Unmount는 크래시하지 않아야 함
    VFS::Get().Unmount("NonExistent");
    // 크래시 없이 통과하면 성공
}

TEST_F(VFSMountEdgeCaseTest, ResolveAfterUnmountFails)
{
    guard.Mount("Temp", temp_dir.temp_path);
    temp_dir.CreateDummyFile("file.txt");

    EXPECT_TRUE(VFS::Resolve(VPath("Temp://file.txt")).HasValue());

    VFS::Get().Unmount("Temp");

    EXPECT_FALSE(VFS::Resolve(VPath("Temp://file.txt")).HasValue());

    // guard의 소멸자에서 이중 Unmount가 발생해도 안전해야 함
}

TEST_F(VFSMountEdgeCaseTest, ExistsReturnsFalseForInvalidVPath)
{
    EXPECT_FALSE(VFS::Exists(VPath{}));
    EXPECT_FALSE(VFS::Exists(VPath("")));
}

TEST_F(VFSMountEdgeCaseTest, ToPathReturnsEmptyForUnmountedScheme)
{
    Path result = VFS::ToPath(VPath("Unknown://file.txt"));
    EXPECT_TRUE(result.IsEmpty());
}


// =============================================================================
// VFS Unicode 경로 테스트
// =============================================================================

class VFSUnicodeTest : public ::testing::Test
{
protected:
    TempDirManager temp_dir{"VFSUnicode"};
    VFSGuard guard{VFS::Get()};

    void SetUp() override
    {
        guard.Mount("Assets", temp_dir.temp_path);
    }
};

TEST_F(VFSUnicodeTest, ResolveUnicodeFileName)
{
    temp_dir.CreateDummyFile("텍스처/플레이어.png");

    VPath vpath("Assets://텍스처/플레이어.png");
    Optional<Path> resolved = VFS::Resolve(vpath);
    ASSERT_TRUE(resolved.HasValue());
    EXPECT_TRUE(resolved->Exists());
}

TEST_F(VFSUnicodeTest, UnresolveUnicodePath)
{
    Path physical = temp_dir.temp_path / Path("モデル/hero.fbx");
    Optional<VPath> vpath = VFS::Unresolve(physical);

    ASSERT_TRUE(vpath.HasValue());
    EXPECT_EQ(vpath->GetScheme(), "Assets");
    EXPECT_EQ(vpath->GetFilename(), "hero.fbx");
}

TEST_F(VFSUnicodeTest, ToPathUnicode)
{
    Path result = VFS::ToPath(VPath("Assets://日本語/ファイル.dat"));
    EXPECT_FALSE(result.IsEmpty());
    EXPECT_EQ(result.FileName().Value(), String("ファイル.dat"));
}


// =============================================================================
// EnsureDirectories 테스트
// =============================================================================

class VFSEnsureDirsTest : public ::testing::Test
{
protected:
    TempDirManager temp_dir{"EnsureDirs"};
    VFSGuard guard{VFS::Get()};
};

TEST_F(VFSEnsureDirsTest, CreatesNonExistentMountDirectory)
{
    Path cache_dir = temp_dir.temp_path / Path("new_cache");
    guard.Mount("Cache", cache_dir);

    EXPECT_FALSE(cache_dir.Exists());

    Array<StringView> schemes;
    schemes.Push("Cache");
    VFS::Get().EnsureDirectories(schemes);

    EXPECT_TRUE(cache_dir.Exists());
    EXPECT_TRUE(cache_dir.IsDirectory());
}

TEST_F(VFSEnsureDirsTest, SkipsAlreadyExistingDirectory)
{
    guard.Mount("Existing", temp_dir.temp_path);

    Array<StringView> schemes;
    schemes.Push("Existing");
    // 이미 존재하는 디렉토리 — 크래시 없이 통과
    VFS::Get().EnsureDirectories(schemes);
    EXPECT_TRUE(temp_dir.temp_path.Exists());
}

TEST_F(VFSEnsureDirsTest, SkipsUnmountedScheme)
{
    Array<StringView> schemes;
    schemes.Push("NotMounted");
    // 등록되지 않은 스킴은 무시 — 크래시 없이 통과
    VFS::Get().EnsureDirectories(schemes);
}
