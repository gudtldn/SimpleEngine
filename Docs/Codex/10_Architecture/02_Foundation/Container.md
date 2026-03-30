---
작성일: 2026-03-30
최종 수정일: 2026-03-30
tags:
  - core
작성 완료: true
---
## 0. Container Module Reference

> Header: `#include "SimpleEngine/Core/Container/FooBar.h"`

## 1. Quick Overview

엔진에서 사용되는 컨테이너들의 목록

| Type               | STL Equivalent             | Note                                                     |
| ------------------ | -------------------------- | -------------------------------------------------------- |
| `Array<T>`         | `std::vector<T>`           | 1.5x Growth                                              |
| `FixedArray<T>`    | `std::array<T, N>`         |                                                          |
| `ArrayView<T>`     | `std::span<T>`             |                                                          |
| `Deque<T>`         | `std::deque<T>`            | 내부 컨테이너로 `std::deque`을 사용 중                              |
| `Queue<T>`         | `std::queue<T>`            | `Deque<T>`기반                                             |
| `Stack<T>`         | `std::stack<T>`            | `Deque<T>`기반                                             |
| `PriorityQueue<T>` | `std::priority_queue<T>`   | `Array<T>`에 Heap Sort 기반                                 |
| `String`           | `std::string`              | `Array<T>`기반, UTF-8 문자열. 내부 API에서 ICU4X 라이브러리를 사용하고 있음   |
| `FixedString<N>`   | `std::array<char, N>`      | `FixedArray<char, N>`기반, Annotation에서 NTTP로 문자열을 받을 때 사용 |
| `StringView`       | `std::string_view`         |                                                          |
| `Map<K, V>`        | `std::map<K, V>`           | 내부 컨테이너로 `std::map<K, V>`을 사용 중                          |
| `Set<T>`           | `std::set<T>`              | 내부 컨테이너로 `std::set<T>`을 사용 중                             |
| `HashMap<K, V>`    | `std::unordered_map<K, V>` | 내부 컨테이너로 `std::unordered_map<K, V>`을 사용 중                |
| `HashSet<T>`       | `std::unordered_set<T>`    | 내부 컨테이너로 `std::unordered_set<T>`을 사용 중                   |
| `FlatMap<K, V>`    | `std::flat_map<K, V>`      | `Array<T>`에 Binary Search 기반 (내부적으로 정렬됨)                 |
| `FlatSet<T>`       | `std::flat_set<T>`         | `Array<T>`에 Binary Search 기반 (내부적으로 정렬됨)                 |
| `Optional<T>`      | `std::optional<T>`         |                                                          |
| `Expected<T, E>`   | `std::expected<T, E>`      |                                                          |

## 2. Categorized Reference

### 2.1. Linear Containers (연속 메모리 구조)

- **Targets**: `Array`, `FixedArray`, `ArrayView`, `Deque`, `Queue`, `Stack`, `PriorityQueue`
- **Common Design**:
	- `Array<T>`는 적당히 메모리 효율을 위해 **1.5x Growth Factor**를 채택
	- `Deque`, `Queue`, `Stack`은 현재 `std::deque` 래퍼이나, 추후 고정 크기 블록 할당자로 교체 가능성을 염두에 둠.
	- `PriorityQueue`는 내부적으로 `Array`를 사용하여 힙 구조를 유지.

### 2.2. Associative & Sorted Containers (검색 최적화)

- **Targets**: `Map`, `Set`, `HashMap`, `HashSet`, `FlatMap`, `FlatSet`
- **Decision Logic**:
    - **Node-based (`Map`, `Set`)**: 요소의 삽입/삭제가 빈번하고 반복자 무효화가 방지되어야 하는 경우 사용. (현재는 `std` Wrapper)
    - **Hash-based (`HashMap`, `HashSet`)**: 평균 O(1) 접근이 필요한 경우 사용. 현재 (현재는 `std` Wrapper)
    - **Flat-based (`FlatMap`, `FlatSet`)**: 데이터 크기가 작고 **순회 성능(Cache Locality)이 중요한 경우** 사용. `Array` 기반의 이진 탐색을 수행하므로 삽입 오버헤드가 있음.

### 2.3 Text & String Handling

- **Targets**: `String`, `FixedString`, `StringView`
- **Special Note (ICU4X)**:
    - 엔진 내부 문자열은 모두 **UTF-8**을 표준으로 함.
    - 유니코드 정규화, Locale 기반 정렬 등 복잡한 처리는 `ICU4X` 라이브러리로 처리중
- Why `ICU4X`?
	- 엔진 초창기에는 `utfcpp` 라이브러리를 사용했으나, 순수 인코딩 변환만을 지원하여 고수준 유니코드 처리를 위해 `ICU4C`로 변경함.
	- 하지만 `ICU4C`는 사용하는 기능에 비해 바이너리가 너무 무겁고 빌드 시간이 매우 오래 걸려 대체제를 찾던 중 `ICU4X`를 발견함.
	- `ICU4X`는 Rust로 작성되었지만, 필요한 기능만 모듈별로 선택해 빌드할 수 있어 바이너리 크기를 획기적으로 줄일 수 있었음. 또한 C++용 FFI 바인딩이 깔끔하게 제공되어 기존 엔진의 `String` 아키텍처와 통합하기에 가장 적합한 대안이었음.
	- TODO: 아직은 `ICU4X`의 모든 기능을 컴파일 하는걸로 되어있지만, 추후 수정 예정
