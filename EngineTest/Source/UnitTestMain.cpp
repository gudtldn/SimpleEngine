#include "gtest/gtest.h"
#include "UnitTestEnvironment.h"

#include "SimpleEngine/Core/Concurrency/TaskScheduler.h"
#include "SimpleEngine/Core/Concurrency/ThreadPool.h"


int main(int argc, char* argv[])
{
    // Google Test 초기화
    ::testing::InitGoogleTest(&argc, argv);

    // Environment 추가
    ::testing::AddGlobalTestEnvironment(new ConfigTestEnvironment());

    auto task_scheduler = std::make_unique<se::concurrency::TaskScheduler>(std::this_thread::get_id());

    // 테스트 실행
    const int res = RUN_ALL_TESTS();
    return res;
}
