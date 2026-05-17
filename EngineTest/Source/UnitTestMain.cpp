#include "gtest/gtest.h"
#include "UnitTestEnvironment.h"

#include "SimpleEngine/Core/Logging/LogBackendManager.h"
#include "SimpleEngine/Core/Logging/Backends/ConsoleBackend.h"


int main(int argc, char* argv[])
{
    // Google Test 초기화
    ::testing::InitGoogleTest(&argc, argv);

    // Environment 추가
    ::testing::AddGlobalTestEnvironment(new EngineTestEnvironment());

    // Log Backend 추가
    {
        se::LogBackendManager::Get().AddBackend<se::ConsoleBackend>();
    }

    // 테스트 실행
    const int res = RUN_ALL_TESTS();
    return res;
}
