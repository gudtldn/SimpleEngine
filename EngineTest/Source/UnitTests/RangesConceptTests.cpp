#include "gtest/gtest.h"

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/Deque.h"
#include "SimpleEngine/Core/Container/FlatMap.h"
#include "SimpleEngine/Core/Container/FlatSet.h"
#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Container/HashSet.h"
#include "SimpleEngine/Core/Container/Map.h"
#include "SimpleEngine/Core/Container/Set.h"

// 컴파일 타임 정적 검증
// 모든 컨테이너가 기대하는 ranges concept을 만족하는지 확인합니다.
static_assert(std::ranges::contiguous_range<se::Array<int>>);
static_assert(std::ranges::random_access_range<se::Array<int>>);
static_assert(std::ranges::random_access_range<se::Deque<int>>);
static_assert(std::ranges::bidirectional_range<se::Map<int, int>>);
static_assert(std::ranges::random_access_range<se::FlatMap<int, int>>);
static_assert(std::ranges::forward_range<se::HashMap<int, int>>);
static_assert(std::ranges::bidirectional_range<se::Set<int>>);
static_assert(std::ranges::bidirectional_range<se::FlatSet<int>>);
static_assert(std::ranges::forward_range<se::HashSet<int>>);

// 빈 테스트 스위트
// static_assert가 통과하면 빌드 성공
TEST(RangesConceptTest, StaticAssertionsPass)
{
    SUCCEED();
}
