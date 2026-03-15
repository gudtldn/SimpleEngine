#pragma once
#include "gtest/gtest.h"
#include "SimpleEngine/Core/FileSystem/VFS.h"
#include "SimpleEngine/Core/FileSystem/FileSystem.h"


// ConfigTest에서 사용하는 Environment
class ConfigTestEnvironment : public ::testing::Environment
{
public:
    virtual void SetUp() override
    {
        // 모든 테스트가 실행되기 전에 딱 한 번 호출됩니다.
        se::VFS::Get().Mount("Config", se::FileSystem::Absolute("Config"));
    }

    virtual void TearDown() override
    {
        // 모든 테스트가 끝난 후에 딱 한 번 호출됩니다.
        se::VFS::Get().Unmount("Config");
    }
};
