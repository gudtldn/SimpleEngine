#include "gtest/gtest.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <thread>
#include <utility>

#include "SimpleEngine/Asset/AssetManager.h"
#include "SimpleEngine/Core/Concurrency/ThreadPool.h"

using namespace se::asset;
using namespace se::core::concurrency;
using namespace se::core::concurrency::coroutine;
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
struct DummyAsset
{
    int value;
};

template <>
class se::asset::AssetLoader<DummyAsset>
{
public:
    [[nodiscard]] Task<std::shared_ptr<DummyAsset>> Load(const std::filesystem::path& path) const
    {
        // Simulate file read and parsing
        if (!std::filesystem::exists(path))
        {
            co_return nullptr;
        }

        std::ifstream file(path);
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
    virtual void SetUp() override
    {
        temp_dir_path = std::filesystem::temp_directory_path() / "AssetManagerTest";
        std::filesystem::create_directories(temp_dir_path);
        path_resolver.Mount("TestAssets", temp_dir_path);
    }

    virtual void TearDown() override
    {
        path_resolver.Unmount("TestAssets");
        std::filesystem::remove_all(temp_dir_path);
    }

    std::filesystem::path temp_dir_path;
    AssetManager asset_manager;
};

TEST_F(AssetManagerTest, LoadSynchronousReturnsValidAsset)
{
    // Setup
    const VPath asset_path = "TestAssets://my_asset.dummy";
    {
        const auto physical_path = path_resolver.Resolve(asset_path, false).Value();
        std::ofstream file(physical_path);
        file << 123;
        file.close();
    }

    // Action
    std::shared_ptr<DummyAsset> asset = asset_manager.LoadSynchronous<DummyAsset>(asset_path);

    // Assert
    ASSERT_NE(asset, nullptr);
    EXPECT_EQ(asset->value, 123);
}

TEST_F(AssetManagerTest, LoadAsyncWaitsForCompletion)
{
    // Setup
    const VPath asset_path = "TestAssets://my_async_asset.dummy";
    {
        const auto physical_path = PathResolver::Get().Resolve(asset_path, false).Value();
        std::ofstream file(physical_path);
        file << 456;
        file.close();
    }

    // Action
    bool is_set = false;
    std::shared_ptr<DummyAsset> asset;
    asset_manager.LoadAsync<DummyAsset>(asset_path, [&asset, &is_set](std::shared_ptr<DummyAsset> in_asset)
    {
        asset = std::move(in_asset);
        is_set = true;
    });

    // Assert
    const TaskSchedulerTest test{ TaskScheduler::Get() };
    RequireConditionTimeout([&] -> bool
    {
        test.ProcessMainThreadTasks();
        return is_set;
    }, std::chrono::seconds(1));

    ASSERT_NE(asset, nullptr);
    EXPECT_EQ(asset->value, 456);
}

TEST_F(AssetManagerTest, LoadSynchronousOnNonExistentFileReturnsNull)
{
    const VPath non_existent_path = "TestAssets://i_dont_exist.dummy";

    std::shared_ptr<DummyAsset> asset = asset_manager.LoadSynchronous<DummyAsset>(non_existent_path);
    EXPECT_EQ(asset, nullptr);
}

TEST_F(AssetManagerTest, LoadAsyncOnNonExistentFileReturnsNull)
{
    const VPath non_existent_path = "TestAssets://i_dont_exist.dummy";

    bool is_set = false;
    std::shared_ptr<DummyAsset> asset;
    asset_manager.LoadAsync<DummyAsset>(non_existent_path, [&asset, &is_set](std::shared_ptr<DummyAsset> in_asset)
    {
        asset = std::move(in_asset);
        is_set = true;
    });

    const TaskSchedulerTest test{ TaskScheduler::Get() };
    RequireConditionTimeout([&] -> bool
    {
        test.ProcessMainThreadTasks();
        return is_set;
    }, std::chrono::milliseconds(100));

    EXPECT_EQ(asset, nullptr);
}

TEST_F(AssetManagerTest, LoadSynchronousOnInvalidVPathReturnsNull)
{
    const VPath invalid_vpath = "InvalidScheme://some_asset.dummy";
    std::shared_ptr<DummyAsset> asset = asset_manager.LoadSynchronous<DummyAsset>(invalid_vpath);
    EXPECT_EQ(asset, nullptr);
}
