#include "doctest/doctest.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <thread>
#include <utility>

#include "SimpleEngine/Asset/AssetManager.h"

using namespace se::asset;
using namespace se::core::concurrency;
using namespace se::core::concurrency::coroutine;


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
            REQUIRE_MESSAGE(false, std::format("Timeout: Condition was not met within {} seconds.", timeout_duration.count()));
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    // 조건 충족
    REQUIRE_MESSAGE(true, "Condition met successfully within the timeout period.");
}

se::utility::PathResolver& path_resolver = se::utility::PathResolver::Get();
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
        path_resolver.Mount("TestAssets", temp_dir_path);
    }

    ~TestFixture()
    {
        path_resolver.Unmount("TestAssets");
        std::filesystem::remove_all(temp_dir_path);
        // Reset the singleton or clear its state if necessary. For now, we assume it's okay.
    }
};

TEST_CASE_FIXTURE(TestFixture, "Load and Get Asset Synchronously")
{
    // Setup a dummy asset file
    const VPath asset_path = "TestAssets://my_asset.dummy";
    {
        const auto physical_path = path_resolver.Resolve(asset_path, false).Value();
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
    const VPath asset_path = "TestAssets://my_async_asset.dummy";
    const auto physical_path = path_resolver.Resolve(asset_path, false).Value();
    {
        std::ofstream file(physical_path);
        file << 456;
    }

    SUBCASE("Load and GetAsset waits for the asset to be loaded")
    {
        bool is_set = false;

        std::shared_ptr<DummyAsset> asset;
        asset_manager.LoadAsync<DummyAsset>(asset_path, [&asset, &is_set]<typename T>(std::shared_ptr<T> in_asset) -> void
        {
            asset = std::move(in_asset);
            is_set = true;
        });

        const TaskSchedulerTest test{ TaskScheduler::Get() };
        RequireConditionTimeout([&] -> bool
        {
            test.ProcessMainThreadTasks();
            return is_set;
        }, std::chrono::seconds(1));

        REQUIRE(asset != nullptr);
        CHECK(asset->value == 456);
    }
}

TEST_CASE_FIXTURE(TestFixture, "Loading non-existent asset")
{
    const VPath non_existent_path = "TestAssets://i_dont_exist.dummy";

    SUBCASE("LoadSynchronous on non-existent file returns nullptr")
    {
        std::shared_ptr<DummyAsset> asset = asset_manager.LoadSynchronous<DummyAsset>(non_existent_path);
        CHECK(asset == nullptr);
    }

    SUBCASE("Load and GetAsset on non-existent file returns nullptr")
    {
        bool is_set = false;

        std::shared_ptr<DummyAsset> asset;
        asset_manager.LoadAsync<DummyAsset>(non_existent_path, [&asset, &is_set]<typename T>(std::shared_ptr<T> in_asset) -> void
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

        CHECK(asset == nullptr);
    }
}

TEST_CASE_FIXTURE(TestFixture, "Loading invalid virtual path")
{
    const VPath invalid_vpath = "InvalidScheme://some_asset.dummy";

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
