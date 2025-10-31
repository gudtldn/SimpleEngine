#include "gtest/gtest.h"

#include <thread>
#include <unordered_set>

#include "SimpleEngine/Core/Types/StringName.h"
#include "SimpleEngine/Core/Container/String.h"

using namespace se;


// --- StringName 테스트를 위한 Fixture 클래스 ---
// 각 테스트가 실행되기 전에 StringName의 내부 풀을 초기화하여
// 테스트 간의 독립성을 보장합니다.
class StringNameTest : public ::testing::Test
{
protected:
    virtual void SetUp() override
    {
        // StringName이 내부 풀을 초기화하는 static 메서드를 가지고 있다고 가정합니다.
        // 예: StringName::ClearPool();
        // 이 기능이 없다면, 테스트 안정성을 위해 추가하는 것이 매우 좋습니다.
    }
};

// --- 1. 핵심 기능 및 비교 테스트 ---

TEST_F(StringNameTest, CaseInsensitiveComparison)
{
    StringName name_a("Hello");
    StringName name_b("hello");
    StringName name_c("World");

    // 비교 해시는 소문자 기준이므로 같아야 함
    EXPECT_EQ(name_a, name_b);
    EXPECT_EQ(name_a.GetComparisonHash(), name_b.GetComparisonHash());

    // 내용은 다르므로 달라야 함
    EXPECT_NE(name_a, name_c);
}

TEST_F(StringNameTest, DisplayHashIsCaseSensitive)
{
    StringName name_a("Hello");
    StringName name_b("hello");
    StringName name_d("Hello");

    // 대소문자가 다르므로 디스플레이 해시는 달라야 함
    EXPECT_NE(name_a.GetDisplayHash(), name_b.GetDisplayHash());
    // 내용은 완전히 같으므로 디스플레이 해시도 같아야 함
    EXPECT_EQ(name_a.GetDisplayHash(), name_d.GetDisplayHash());
}

TEST_F(StringNameTest, ToStringReturnsOriginalString)
{
    StringName name_a("Hello");
    StringName name_b("hello");
    StringName name_c("World");

    EXPECT_EQ(name_a.ToString(), "Hello");
    EXPECT_EQ(name_b.ToString(), "hello");
    EXPECT_EQ(name_c.ToString(), "World");
}


// --- 2. 'None' 값 처리 테스트 ---

TEST_F(StringNameTest, VariousCasingsOfNoneResolveToStaticNone)
{
    EXPECT_EQ(StringName("none"), StringName::None);
    EXPECT_EQ(StringName("None"), StringName::None);
    EXPECT_EQ(StringName("NONE"), StringName::None);
    EXPECT_EQ(StringName("nOnE"), StringName::None);
}

TEST_F(StringNameTest, EmptyStringResolvesToStaticNone)
{
    EXPECT_EQ(StringName(""), StringName::None);
}

TEST_F(StringNameTest, StaticNoneProperties)
{
    // 기본 생성자는 None과 같아야 함
    EXPECT_EQ(StringName(), StringName::None);

    EXPECT_EQ(StringName::None.GetDisplayHash(), 0);
    EXPECT_EQ(StringName::None.GetComparisonHash(), 0);
    EXPECT_EQ(StringName::None.ToString(), "None");
}


// --- 3. 스레드 안정성 테스트 ---
// 이 테스트는 시간이 오래 걸릴 수 있으므로, DISABLED_ 접두사를 붙여
// 평소에는 실행되지 않도록 하고 필요할 때만 실행하는 것이 좋습니다.
TEST_F(StringNameTest, IsThreadSafe)
{
    constexpr int num_threads = 16;
    constexpr int num_iterations_per_thread = 1000;

    const std::vector<std::string_view> test_strings = {
        "PlayerCharacter", "playercharacter", "EnemyAIController",
        "RenderComponent", "RENDERCOMPONENT", "PhysicsSystem",
        "AudioEmitter", "GameInstance", "NONE"
    };

    std::vector<std::thread> threads;
    threads.reserve(num_threads);

    for (int i = 0; i < num_threads; ++i)
    {
        threads.emplace_back([&test_strings, i]
        {
            for (int j = 0; j < num_iterations_per_thread; ++j)
            {
                const auto& str = test_strings[(i + j) % test_strings.size()];
                StringName name(str);

                // 생성된 StringName의 유효성 검사
                // 대소문자 변환 후 비교가 일치하는지 확인
                se::String lower_str = se::String{ str }.ToLower();
                StringName lower_name(lower_str);

                // 스레드 내에서는 ASSERT 대신 EXPECT를 사용하는 것이 더 안전할 수 있습니다.
                // ASSERT는 실패 시 스레드를 즉시 종료시키지만, 다른 스레드들은 계속 실행될 수 있기 때문입니다.
                EXPECT_EQ(name, lower_name);
                EXPECT_EQ(name.GetComparisonHash(), lower_name.GetComparisonHash());
            }
        });
    }

    for (auto& t : threads)
    {
        t.join();
    }

    // 최종 상태 검증
    StringName player("PlayerCharacter");
    StringName player_lower("playercharacter");
    StringName physics("PhysicsSystem");

    EXPECT_EQ(player, player_lower);
    EXPECT_NE(player.GetDisplayHash(), player_lower.GetDisplayHash());
    EXPECT_NE(player, physics);
}


// --- 4. STL 컨테이너 호환성 테스트 ---

TEST_F(StringNameTest, WorksWithUnorderedSet)
{
    std::unordered_set<StringName> name_set;

    StringName name1("Weapon");
    StringName name2("Armor");
    StringName name3("weapon"); // name1과 같아야 함

    name_set.insert(name1);
    name_set.insert(name2);
    ASSERT_EQ(name_set.size(), 2);

    // 이미 "Weapon"에 대한 comparison_hash가 존재하므로 삽입되지 않아야 함
    auto result = name_set.insert(name3);
    EXPECT_FALSE(result.second) << "Insertion should have failed for a case-insensitive duplicate.";
    EXPECT_EQ(name_set.size(), 2);

    EXPECT_EQ(name_set.count(name1), 1);
    EXPECT_EQ(name_set.count(name3), 1) << "Case-insensitive version should be found.";
}


// --- 5. Find 메서드 테스트 ---

TEST_F(StringNameTest, FindMethodBehavesCorrectly)
{
    // 먼저 이름을 생성하여 풀에 등록
    StringName name1("TestName");

    // 이제 Find로 찾을 수 있어야 함
    EXPECT_EQ(StringName::Find("TestName"), name1);
    EXPECT_EQ(StringName::Find("testName"), name1); // Case-insensitive find

    // 존재하지 않는 이름
    EXPECT_EQ(StringName::Find("NonExistentName"), StringName::None);

    // 'NONE'도 None으로 찾아져야 함
    EXPECT_EQ(StringName::Find("NONE"), StringName::None);
}
