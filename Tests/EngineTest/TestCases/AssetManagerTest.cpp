// #define DOCTEST_CONFIG_DISABLE
#include "doctest.h"
#include "tracy/Tracy.hpp"

import std;
import SE.Assets;
import SE.Core;
import SE.Types;

using namespace se::assets;
using namespace se::core;
using namespace se::core::concurrency::coroutine;


// 1. Dummy Asset & Loader for testing
struct DummyAsset
{
    int value;
};

template <>
class loaders::AssetLoader<DummyAsset>
{
public:
    Task<std::shared_ptr<DummyAsset>> Load(const std::filesystem::path& path) const
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

TEST_SUITE("SimpleEngine.Assets.AssetManager")
{
struct TestFixture
{
    std::filesystem::path temp_dir_path;
    AssetManager asset_manager;

    TestFixture()
    {
        temp_dir_path = std::filesystem::temp_directory_path() / "AssetManagerTest";
        std::filesystem::create_directories(temp_dir_path);
        paths::PathResolver::Get().Mount(u8"TestAssets", temp_dir_path);
    }

    ~TestFixture()
    {
        paths::PathResolver::Get().Unmount(u8"TestAssets");
        std::filesystem::remove_all(temp_dir_path);
        // Reset the singleton or clear its state if necessary. For now, we assume it's okay.
    }
};

TEST_CASE_FIXTURE(TestFixture, "Load and Get Asset Synchronously")
{
    // Setup a dummy asset file
    const VPath asset_path = u8"TestAssets://my_asset.dummy";
    const auto physical_path = paths::Resolve(asset_path).Value();
    {
        std::ofstream file(physical_path);
        file << 123;
    }

    SUBCASE("LoadSynchronous returns a valid asset")
    {
        std::shared_ptr<DummyAsset> asset = asset_manager.LoadSynchronous<DummyAsset>(asset_path);
        REQUIRE(asset != nullptr);
        CHECK(asset->value == 123);
    }
}

TEST_CASE_FIXTURE(TestFixture, "Load and Get Asset Asynchronously")
{
    const VPath asset_path = u8"TestAssets://my_async_asset.dummy";
    const auto physical_path = paths::Resolve(asset_path).Value();
    {
        std::ofstream file(physical_path);
        file << 456;
    }

    SUBCASE("Load and GetAsset waits for the asset to be loaded")
    {
        std::shared_ptr<DummyAsset> asset;
        asset_manager.LoadAsync<DummyAsset>(asset_path, [&asset]<typename T>(std::shared_ptr<T> in_asset) -> void
        {
            asset = in_asset;
        });

        using namespace std::chrono_literals;
        std::this_thread::sleep_for(500ms); // Wait for the async loading to finish (500 ms)

        const concurrency::TaskSchedulerTest test{ concurrency::TaskScheduler::Get() };
        test.ProcessMainThreadTasks();

        REQUIRE(asset != nullptr);
        CHECK(asset->value == 456);
    }
}

TEST_CASE_FIXTURE(TestFixture, "Loading non-existent asset")
{
    const VPath non_existent_path = u8"TestAssets://i_dont_exist.dummy";

    SUBCASE("LoadSynchronous on non-existent file returns nullptr")
    {
        std::shared_ptr<DummyAsset> asset = asset_manager.LoadSynchronous<DummyAsset>(non_existent_path);
        CHECK(asset == nullptr);
    }

    SUBCASE("Load and GetAsset on non-existent file returns nullptr")
    {
        std::shared_ptr<DummyAsset> asset;
        asset_manager.LoadAsync<DummyAsset>(non_existent_path, [&asset]<typename T>(std::shared_ptr<T> in_asset) -> void
        {
            asset = in_asset;
        });

        using namespace std::chrono_literals;
        std::this_thread::sleep_for(500ms); // Wait for the async loading to finish (500 ms)

        const concurrency::TaskSchedulerTest test{ concurrency::TaskScheduler::Get() };
        test.ProcessMainThreadTasks();

        CHECK(asset == nullptr);
    }
}

TEST_CASE_FIXTURE(TestFixture, "Loading invalid virtual path")
{
    const VPath invalid_vpath = u8"InvalidScheme://some_asset.dummy";

    SUBCASE("LoadSynchronous on invalid VPath returns nullptr")
    {
        std::shared_ptr<DummyAsset> asset = asset_manager.LoadSynchronous<DummyAsset>(invalid_vpath);
        CHECK(asset == nullptr);
    }

    // SUBCASE("Load on invalid VPath returns nullopt")
    // {
    //     Optional<AssetHandle<DummyAsset>> handle_opt = asset_manager.Load<DummyAsset>(invalid_vpath);
    //     CHECK_FALSE(handle_opt.HasValue());
    // }
}
}
