---
작성일: 2026-03-18
tags:
  - rendering
  - graphics
  - core
---
# 1. 개요

GPU Draw Call은 호출 순서대로 처리되는데, 매 Draw마다 VB(Vertex Buffer) / IB(Index Buffer) 바인딩이나, PSO(Pipeline State Object)가 바뀌면 GPU 내부에 **State Change** 비용이 발생하게 된다.

이를 방지하기 위해서 같은 Mesh나 Material끼리 묶어서 연속으로 그리면 전체적으로 **State Change** 비용이 줄어들게 된다.

예시:

* 정렬 전: \[나무A, 바위A, 나무B, 바위B, 나무C\] → VB 교체 5회
* 정렬 후: \[나무A, 나무B, 나무C, 바위A, 바위B\] → VB 교체 2회

# 2. 어떻게 구현하는가?

## 2.1. 데이터 구조

> SimpleEngine/EngineCore/Include/SimpleEngine/Graphics/Scene/DrawCommand.h

```c++
struct DrawCommand
{
    Matrix4x4 model_matrix;     // 모델 -> 월드 변환 행렬
    asset::AssetId mesh_id;     // 그리려는 Mesh의 ID
    asset::AssetId material_id; // 그리려는 Material의 ID
    uint64 sort_key = 0;        // 정렬용 비트 패킹 키
};
```

여기서 `uint64`가 정렬의 중요한 역할을 하게된다.

## 2.2. Bit Packing을 이용한 효율적 정렬

`uint64` 타입의 `sort_key` 하나에 여러 정렬 조건을 밀어 넣어, **비교 연산 1번으로 두 조건을 동시에 정렬할 수 있다.**

이렇게까지 하는 이유는 각각의 데이터를 조건문으로 비교하는 것보다, `uint64` 정수 비교 1번이 훨씬 CPU 캐시 적중과 분기 예측에 유리하기 때문. 즉, 매 프레임 호출되는 렌더링 로직에서 조금이라도 더 빠르게 정렬을 하기 위함이다.

> `uint64`의 비트 패킹 구조
> SimpleEngine/EngineCore/Source/Graphics/Scene/CollectDrawData.cpp

```
  63               32 31                0
  ┌──────────────────┬──────────────────┐
  │ material_id 해시  │   mesh_id 해시   │
  └──────────────────┴──────────────────┘
       1차 정렬 키          2차 정렬 키
```

| **비트 범위**        | **데이터 (필드)**     | **설명**                             |
| ---------------- | ---------------- | ---------------------------------- |
| **63 ~ 32** (상위) | `material_id` 해시 | **1차 정렬 기준:** 머티리얼이 같아야 PSO 전환이 적음 |
| **31 ~ 0** (하위)  | `mesh_id` 해시     | **2차 정렬 기준:** 머티리얼이 같을 때 메쉬별로 모음   |

## 2.3. 왜 우선순위을 이렇게 뒀는가?

상위 32비트를 `material_id`의 해시로 설정한 이유는 Material의 상태 변경(PSO 교체 + 셰이더/텍스처 바인딩 변경)이 Mesh 교체(VB/IB 재바인딩) 보다 훨씬 비싸기 때문. 따라서 더 비싼 연산을 1차 키로 삼아서 최우선으로 비교.
