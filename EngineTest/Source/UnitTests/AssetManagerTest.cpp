#include "gtest/gtest.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <thread>
#include <utility>

#include "SimpleEngine/Asset/AssetManager.h"
#include "SimpleEngine/Core/Concurrency/ThreadPool.h"

using namespace se;
using namespace se::asset;
using namespace se::concurrency;
using namespace se::utility;


namespace
{
template <typename Fn>
    requires std::predicate<Fn>
void RequireConditionTimeout(Fn&& condition, std::chrono::milliseconds timeout_duration)
{
    using namespace std::chrono_literals;
    const auto start_time = std::chrono::steady_clock::now();

    while (!std::forward<Fn>(condition)())
    {
        const auto elapsed_time = std::chrono::steady_clock::now() - start_time;
        if (elapsed_time >= timeout_duration)
        {
            // Timeout
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    // 최종 검증
    if (std::forward<Fn>(condition)())
    {
        // 조건 충족
        SUCCEED();
    }
    else
    {
        // Timeout 발생
        FAIL() << "Timeout: Condition was not met within " << timeout_duration.count() << " milliseconds.";
    }
}

PathResolver& path_resolver = PathResolver::Get();
}

// 1. Dummy Asset & Loader for testing
class DummyAsset : public IAsset
{
public:
    int value;
};

class DummyAssetLoader : public IAssetLoader
{
public:
    virtual Task<std::shared_ptr<IAsset>> Load(
        const std::filesystem::path& physical_path,
        [[maybe_unused]] const IAssetImportSettings* import_settings
    ) override
    {
        // Simulate file read and parsing
        if (!std::filesystem::exists(physical_path))
        {
            co_return nullptr;
        }

        std::ifstream file(physical_path);
        int file_content;
        file >> file_content;

        // Simulate async loading time
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(250ms);

        auto asset = std::make_shared<DummyAsset>();
        asset->value = file_content;
        co_return asset;
    }
};

class AssetManagerTest : public ::testing::Test
{
protected:
    AssetHandle<DummyAsset> RegisterDummyAsset(const StringName& name, int content)
    {
        using namespace se::asset;
        using namespace se::core;

        // 1. 새 GUID 생성
        Guid guid = Guid::NewGuid();
        VPath vpath = se::String::Format("TestAssets://{}", name.ToString());

        // 2. AssetRegistry에 등록
        asset_manager.GetRegistry().AddEntry({
            .guid = guid,
            .asset_type = se::refl::TypeId::Get<DummyAsset>(),
            .loader_type = se::refl::TypeId::Get<DummyAssetLoader>(),
            .virtual_path = vpath
        });

        // 3. 실제 파일 생성
        const auto physical_path = path_resolver.Resolve(vpath, false).Value();
        std::ofstream file(physical_path);
        file << content;
        file.close();

        // 4. 생성된 핸들 반환
        return AssetHandle<DummyAsset>{ guid };
    }

    virtual void SetUp() override
    {
        temp_dir_path = std::filesystem::temp_directory_path() / "AssetManagerTest";
        std::filesystem::create_directories(temp_dir_path);
        path_resolver.Mount("TestAssets", temp_dir_path);

        // AssetManager에 로더 등록
        asset_manager.RegisterLoader<DummyAsset, DummyAssetLoader>("dummy");
    }

    virtual void TearDown() override
    {
        path_resolver.Unmount("TestAssets");
        std::filesystem::remove_all(temp_dir_path);
    }

protected:
    std::filesystem::path temp_dir_path;
    AssetManager asset_manager;
};

TEST_F(AssetManagerTest, LoadSynchronousReturnsValidAsset)
{
    // Arrange (Given)
    AssetHandle<DummyAsset> handle = RegisterDummyAsset("my_asset.dummy", 123);

    // Act (When)
    std::shared_ptr<DummyAsset> asset = asset_manager.LoadSynchronous<DummyAsset>(handle);

    // Assert (Then)
    ASSERT_NE(asset, nullptr);
    EXPECT_EQ(asset->value, 123);
}

TEST_F(AssetManagerTest, LoadAsyncWaitsForCompletion)
{
    // Arrange
    AssetHandle<DummyAsset> handle = RegisterDummyAsset("my_async_asset.dummy", 456);
    bool is_set = false;
    std::shared_ptr<DummyAsset> asset;

    // Act
    asset_manager.LoadAsync<DummyAsset>(handle, [&asset, &is_set](std::shared_ptr<DummyAsset> in_asset)
    {
        asset = std::move(in_asset);
        is_set = true;
    });

    // Assert
    const TaskSchedulerTest test_helper{ TaskScheduler::Get() };
    RequireConditionTimeout([&] -> bool
    {
        test_helper.ProcessMainThreadTasks();
        return is_set;
    }, std::chrono::seconds(1));

    ASSERT_NE(asset, nullptr);
    EXPECT_EQ(asset->value, 456);
}

TEST_F(AssetManagerTest, LoadSynchronousOnInvalidHandleReturnsNull)
{
    // Arrange: 유효하지 않은 (기본 생성된) 핸들
    AssetHandle<DummyAsset> invalid_handle;

    // Act
    std::shared_ptr<DummyAsset> asset = asset_manager.LoadSynchronous<DummyAsset>(invalid_handle);

    // Assert
    EXPECT_EQ(asset, nullptr);
}

TEST_F(AssetManagerTest, LoadSynchronousOnRegisteredButNonExistentFileReturnsNull)
{
    // Arrange: 레지스트리에는 등록하지만, 실제 파일은 만들지 않음
    AssetHandle<DummyAsset> handle;
    {
        handle = AssetHandle<DummyAsset>(Guid::NewGuid());
        asset_manager.GetRegistry().AddEntry({
            .guid = handle.GetGuid(),
            .asset_type = se::refl::TypeId::Get<DummyAsset>(),
            .loader_type = se::refl::TypeId::Get<DummyAssetLoader>(),
            .virtual_path = "TestAssets://i_dont_exist.dummy"
        });
    }

    // Act
    std::shared_ptr<DummyAsset> asset = asset_manager.LoadSynchronous<DummyAsset>(handle);

    // Assert
    EXPECT_EQ(asset, nullptr);
}

TEST_F(AssetManagerTest, LoadAsyncOnNonExistentFileReturnsNull)
{
    // Arrange
    AssetHandle<DummyAsset> handle;
    {
        handle = AssetHandle<DummyAsset>(Guid::NewGuid());
        asset_manager.GetRegistry().AddEntry({
            .guid = handle.GetGuid(),
            .asset_type = se::refl::TypeId::Get<DummyAsset>(),
            .loader_type = se::refl::TypeId::Get<DummyAssetLoader>(),
            .virtual_path = "TestAssets://i_dont_exist_async.dummy"
        });
    }

    bool is_set = false;
    std::shared_ptr<DummyAsset> asset;

    // Act
    asset_manager.LoadAsync<DummyAsset>(handle, [&asset, &is_set](std::shared_ptr<DummyAsset> in_asset)
    {
        asset = std::move(in_asset);
        is_set = true;
    });

    // Assert
    const TaskSchedulerTest test_helper{ TaskScheduler::Get() };
    RequireConditionTimeout([&] -> bool
    {
        test_helper.ProcessMainThreadTasks();
        return is_set;
    }, std::chrono::seconds(1));

    EXPECT_EQ(asset, nullptr);
}
