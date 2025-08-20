#include "doctest.h"

import SimpleEngine.Prelude;
import std;


TEST_SUITE("SimpleEngine.Core.StringName")
{
using namespace std::string_view_literals;


TEST_CASE("1. Core Functionality and Comparisons")
{
    // 테스트 시작 전 풀을 초기화하는 것이 좋습니다 (테스트 간 독립성).
    // 여기서는 Get()을 통해 싱글턴에 접근하므로 상태가 유지됩니다.

    StringName name_a(u8"Hello");
    StringName name_b(u8"hello");
    StringName name_c(u8"World");
    StringName name_d(u8"Hello");

    SUBCASE("Case-Insensitive Comparison")
    {
        // 비교 해시는 소문자 기준으로 생성되므로 같아야 함
        CHECK(name_a == name_b);
        CHECK(name_a.GetComparisonHash() == name_b.GetComparisonHash());

        // 내용은 다르므로 당연히 달라야 함
        CHECK(name_a != name_c);
        CHECK(name_b != name_c);
    }

    SUBCASE("Display Hash vs Comparison Hash")
    {
        // 대소문자가 다르므로 디스플레이 해시는 달라야 함
        CHECK(name_a.GetDisplayHash() != name_b.GetDisplayHash());
        // 내용은 완전히 같으므로 디스플레이 해시도 같아야 함
        CHECK(name_a.GetDisplayHash() == name_d.GetDisplayHash());
    }

    SUBCASE("ToString functionality")
    {
        // ToString()은 원본 문자열을 반환해야 함
        CHECK(name_a.ToString() == u8"Hello");
        CHECK(name_b.ToString() == u8"hello");
        CHECK(name_c.ToString() == u8"World");
    }
}

TEST_CASE("2. 'None' Value Handling")
{
    SUBCASE("Various casings of 'none' should resolve to StringName::None")
    {
        StringName none1(u8"none");
        StringName none2(u8"None");
        StringName none3(u8"NONE");
        StringName none4(u8"nOnE");

        CHECK(none1 == StringName::None);
        CHECK(none2 == StringName::None);
        CHECK(none3 == StringName::None);
        CHECK(none4 == StringName::None);

        CHECK(none1.GetDisplayHash() == 0);
        CHECK(none1.GetComparisonHash() == 0);
    }

    SUBCASE("Empty string should resolve to StringName::None")
    {
        StringName empty_name(u8"");
        CHECK(empty_name == StringName::None);
        CHECK(empty_name.GetDisplayHash() == 0);
    }

    SUBCASE("StringName::None properties")
    {
        // 기본 생성자는 None과 같아야 함
        StringName default_constructed;
        CHECK(default_constructed == StringName::None);

        CHECK(StringName::None.GetDisplayHash() == 0);
        CHECK(StringName::None.GetComparisonHash() == 0);
        CHECK(StringName::None.ToString() == u8"None");
    }
}

TEST_CASE("3. Thread Safety with Reader-Writer Lock")
{
    // 이 테스트는 shared_mutex 구현의 정확성을 검증합니다.
    // 여러 스레드가 동시에 읽기(shared_lock) 및 쓰기(unique_lock)를 시도합니다.

    constexpr int num_threads = 16;
    constexpr int num_iterations_per_thread = 1000;

    std::vector<std::thread> threads;

    // 여러 스레드에서 공유하며 사용할 문자열 데이터
    const std::vector<std::u8string_view> test_strings = {
        u8"PlayerCharacter", u8"playercharacter", u8"EnemyAIController",
        u8"RenderComponent", u8"RENDERCOMPONENT", u8"PhysicsSystem",
        u8"AudioEmitter", u8"GameInstance", u8"NONE" // 'None' 케이스 포함
    };

    for (int i = 0; i < num_threads; ++i)
    {
        threads.emplace_back([&test_strings, num_iterations_per_thread, i]
        {
            for (int j = 0; j < num_iterations_per_thread; ++j)
            {
                // 스레드마다 접근하는 문자열을 다르게 하여 읽기와 쓰기가 혼합되도록 함
                const auto& str = test_strings[(i + j) % test_strings.size()];

                // StringName 생성 (내부적으로 FindOrEmplace 호출)
                StringName name(str);

                // 생성된 StringName의 유효성 검사
                if (str == u8"NONE" || str == u8"none")
                {
                    REQUIRE(name == StringName::None);
                }
                else
                {
                    // 대소문자 변환 후 비교가 일치하는지 확인
                    std::u8string lower_str;
                    for (char8 c : str) { lower_str += static_cast<char8>(std::tolower(c)); }

                    StringName lower_name(lower_str);
                    REQUIRE(name == lower_name);
                    REQUIRE(name.GetComparisonHash() == lower_name.GetComparisonHash());
                }
            }
        });
    }

    // 모든 스레드가 작업을 마칠 때까지 대기
    for (auto& t : threads)
    {
        t.join();
    }

    // 스레드 작업 후 최종 상태 검증
    // 데이터 레이스 컨디션이 있었다면 이 값들이 깨졌을 수 있음
    StringName player(u8"PlayerCharacter");
    StringName player_lower(u8"playercharacter");
    StringName physics(u8"PhysicsSystem");

    CHECK(player == player_lower);
    CHECK(player.GetDisplayHash() != player_lower.GetDisplayHash());
    CHECK(player != physics);
}

TEST_CASE("4. STL Container Compatibility")
{
    // std::hash 특수화가 잘 동작하는지 확인
    std::unordered_set<StringName> name_set;

    StringName name1(u8"Weapon");
    StringName name2(u8"Armor");
    StringName name3(u8"weapon"); // name1과 같음

    name_set.insert(name1);
    name_set.insert(name2);

    CHECK(name_set.size() == 2);

    // 이미 "Weapon"에 대한 comparison_hash가 존재하므로 삽입되지 않아야 함
    name_set.insert(name3);
    CHECK(name_set.size() == 2);

    CHECK(name_set.count(name1) == 1);
    CHECK(name_set.count(name2) == 1);
    CHECK(name_set.count(name3) == 1); // name3는 name1과 같으므로 존재함
}
}
