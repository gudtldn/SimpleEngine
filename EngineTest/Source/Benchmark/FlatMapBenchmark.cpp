#include <benchmark/benchmark.h>
#include <vector>
#include <random>
#include <algorithm> // for std::shuffle
#include <numeric>   // for std::iota

// 사용자의 엔진 경로에 맞춰 헤더 포함
#include "SimpleEngine/Core/Container/FlatMap.h"
#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Container/Map.h"

namespace se::benchmark_test
{
    // =========================================================================
    // Helper: 랜덤 키 생성 (벤치마크 시간 측정에서 제외하기 위함)
    // =========================================================================
    std::vector<int> GetRandomKeys(int size)
    {
        std::vector<int> keys(size);
        std::iota(keys.begin(), keys.end(), 0); // 0, 1, 2, ... 채우기

        std::mt19937 gen(1234);
        std::shuffle(keys.begin(), keys.end(), gen); // 뒤섞기
        return keys;
    }

    // =========================================================================
    // 1. Insert: Sequential (순차 삽입) -> FlatMap 최상의 시나리오
    // =========================================================================
    template <typename MapType>
    static void BM_Map_Insert_Sequential(benchmark::State& state)
    {
        for (auto _ : state)
        {
            MapType map;
            // 0, 1, 2... 순서대로 삽입 (재정렬/이동 없음)
            for (int i = 0; i < state.range(0); ++i)
            {
                map.Insert(i, i);
            }
            benchmark::DoNotOptimize(map);
        }
    }

    // =========================================================================
    // 2. Insert: Random (무작위 삽입) -> FlatMap 최악의 시나리오
    // =========================================================================
    template <typename MapType>
    static void BM_Map_Insert_Random(benchmark::State& state)
    {
        // 데이터 준비 (측정 제외)
        auto keys = GetRandomKeys(state.range(0));

        for (auto _ : state)
        {
            MapType map;
            // 무작위 순서 삽입 (FlatMap은 매번 memmove 발생)
            for (int key : keys)
            {
                map.Insert(key, key);
            }
            benchmark::DoNotOptimize(map);
        }
    }

    // =========================================================================
    // 3. Find (검색 성능)
    // =========================================================================
    template <typename MapType>
    static void BM_Map_Find(benchmark::State& state)
    {
        // 1. 맵 구성 (측정 제외)
        MapType map;
        int size = state.range(0);
        auto keys = GetRandomKeys(size);

        for (int key : keys)
        {
            map.Insert(key, key);
        }

        // 2. 검색할 키 목록 준비 (Hit 위주)
        // 측정 루프 안에서 난수를 만들지 않기 위해 미리 생성
        auto lookup_keys = GetRandomKeys(size);
        size_t mask = lookup_keys.size() - 1;

        // 3. 측정 시작
        for (auto _ : state)
        {
            // 루프마다 다른 키를 검색하여 캐시 히트율을 현실적으로 측정
            // state.iterations()는 계속 증가하므로 모듈러 연산 등으로 인덱스 접근
            int key = lookup_keys[state.iterations() % size];
            auto result = map.Find(key);
            benchmark::DoNotOptimize(result);
        }
    }

    // =========================================================================
    // 4. Iteration (순회 성능) -> FlatMap 압도적 우위 (Cache Locality)
    // =========================================================================
    template <typename MapType>
    static void BM_Map_Iteration(benchmark::State& state)
    {
        // 1. 맵 구성 (측정 제외)
        MapType map;
        int size = state.range(0);
        // 순회 성능은 데이터 삽입 순서(메모리 배치)에 영향을 받을 수 있으므로
        // 랜덤 삽입 상태에서 테스트하는 것이 더 리얼함 (Tree의 경우)
        auto keys = GetRandomKeys(size);
        for (int key : keys)
        {
            map.Insert(key, key);
        }

        // 2. 측정 시작
        for (auto _ : state)
        {
            int64_t sum = 0;
            for (auto& pair : map)
            {
                sum += pair.second;
            }
            benchmark::DoNotOptimize(sum);
        }
    }

    // =========================================================================
    // 벤치마크 등록 매크로
    // Range: 8 ~ 8192 (2^3 ~ 2^13)
    // =========================================================================

    #define REGISTER_BENCHMARK(Func, DisplayName) \
        BENCHMARK_TEMPLATE(Func, HashMap<int, int>)->Range(8, 8 << 10)->Name("BM_HashMap_" DisplayName); \
        BENCHMARK_TEMPLATE(Func, Map<int, int>)->Range(8, 8 << 10)->Name("BM_Map_" DisplayName); \
        BENCHMARK_TEMPLATE(Func, FlatMap<int, int>)->Range(8, 8 << 10)->Name("BM_FlatMap_" DisplayName);

    // 1. 순차 삽입
    REGISTER_BENCHMARK(BM_Map_Insert_Sequential, "Insert_Seq");

    // 2. 랜덤 삽입 (FlatMap 성능 저하 확인용)
    REGISTER_BENCHMARK(BM_Map_Insert_Random, "Insert_Rnd");

    // 3. 검색
    REGISTER_BENCHMARK(BM_Map_Find, "Find");

    // 4. 순회 (FlatMap 성능 우위 확인용)
    REGISTER_BENCHMARK(BM_Map_Iteration, "Iter");
}
