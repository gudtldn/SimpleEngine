#pragma once

#include "SimpleEngine/Core/Engine/Engine.h"
#include "SimpleEngine/Core/FileSystem/VFS.h"
#include "SimpleEngine/Core/FileSystem/FileSystem.h"

#include "gtest/gtest.h"


// 엔진 인스턴스를 테스트 기간 동안 유지해주는 Environment
class EngineTestEnvironment : public ::testing::Environment
{
public:
    virtual void SetUp() override
    {
        // 모든 테스트가 실행되기 전에 딱 한 번 호출됩니다.
        se::VFS::Get().Mount("Config", se::FileSystem::Absolute("Config"));

        // Engine 생성자가 내부적으로 instance 포인터를 설정하고 VFS를 초기화합니다.
        engine = std::make_unique<se::Engine>();
    }

    virtual void TearDown() override
    {
        // 모든 테스트가 끝난 후에 딱 한 번 호출됩니다.
        se::VFS::Get().Unmount("Config");

        engine.reset();
    }

private:
    std::unique_ptr<se::Engine> engine;
};
