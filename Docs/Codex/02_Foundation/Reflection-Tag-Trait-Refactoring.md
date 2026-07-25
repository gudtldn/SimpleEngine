---
작성일: 2026-07-25
최종 수정일: 2026-07-25
작성 완료: true
tags:
  - reflection
  - ecs
  - tag-trait
  - macro
  - refactoring
---
# 리플렉션 태그 트레이트 리팩토링 — RegistrationHook에서 TagTraits까지

> [!NOTE]
> 이 문서는 AI(Claude)가 작성했습니다. 세션에서 논의된 설계 결정과 코드 변경 내역을 정리한 것이며,
> 코드가 실제 근거이니 내용이 어긋나 보이면 코드 쪽을 신뢰해 주세요.

> **한 줄 요약:** 리플렉션 코어(`Reflect.h`)가 ECS를 직접 참조하고, 태그 이름을 `if constexpr`
> 체인으로 하드코딩하던 두 가지 층위 위반을 컴파일타임 트레이트 특수화(`TagTraits.h`)로 대체해,
> 코어가 태그의 존재는 알되 의미는 전혀 모르는 상태로 만들었다.

**코드 진입점:**

- `EngineCore/Include/SimpleEngine/Core/Reflection/TagTraits.h` - 4개 트레이트 + 3개 헬퍼 매크로
- `EngineCore/Include/SimpleEngine/Core/Reflection/Annotations.h` - 태그 정의 + 트레이트 특수화 (파일 끝 `namespace se::detail` 블록)
- `EngineCore/Include/SimpleEngine/Core/Reflection/Reflect.h` - `MakeTypeFlags`/`MakePropertyMetadata`/`DispatchRegistrationHooks`
- `EngineCore/Include/SimpleEngine/ECS/ECSReflectionHook.h` - ECS 모듈의 `RegistrationTrait` 특수화

---

## 1. 배경: 무엇이 문제였나

`SE_CLASS`/`SE_REFLECT_PROPERTY` 계열 매크로는 태그(`meta::Hidden`, `meta::Component`,
`meta::DisplayName<"...">` 등)를 NTTP 팩으로 받아 `Reflect.h`의 몇몇 `consteval`/템플릿
함수에서 처리한다. 리팩토링 전에는 두 가지 문제가 겹쳐 있었다.

1. **`Reflect.h`가 `ECS/ECSRegistry.h`를 직접 include**하고, 태그가 `Component`/`Resource`면
   `ECSRegistry::Get().RegisterComponentOps<T>()`를 하드코딩 호출했다. 리플렉션 코어(모든
   타입이 거쳐가는 공용 코드)가 ECS라는 특정 소비자를 알고 있는 층위 위반이었다.
2. **`MakeTypeFlags`/`MakePropertyMetadata`가 `if constexpr (std::same_as<TagType,
   tags::Hidden>) ...` 형태의 분기 체인**이었다. 새 태그를 추가할 때마다 이 공유 로직을
   직접 열어서 분기를 끼워 넣어야 했고, 실수로 안 하면 fallback
   `static_assert(AlwaysFalse<TagType>, ...)`로 컴파일 에러가 나긴 하지만 코어 파일을
   건드리는 건 매번 필요했다.

1번은 이번 세션 이전에 `RegistrationHook`/`HookRequired` 패턴으로 이미 해결되어 있었다.
이번 세션에서는 그 패턴을 실제로 적용해보며 발견한 문제(아래 3장)를 보완하고, 2번 문제를
같은 철학으로 해결했다.

---

## 2. 변경 내용

### 2.1 `RegistrationHook`/`HookRequired` → `RegistrationTrait`/`HookRequiredTrait`

ECS 등록처럼 "일부 태그만 후처리가 필요한" 경우를 위한 확장점. 두 개의 독립된 트레이트로
분리되어 있다 (하나로 합칠 수 없는 이유는 3장 참고):

- `HookRequiredTrait<Tag>::VALUE` — 이 태그가 훅 구현을 **반드시** 가져야 하는지 (기본값 `false`)
- `RegistrationTrait<Tag>::Apply<T>()` — 실제 훅 구현. primary 템플릿은 비어 있고,
  `requires { RegistrationTrait<Tag>::template Apply<T>(); }`로 "구현됐는지"를 판단한다.

`Reflect.h`의 `DispatchRegistrationHooks`는 이 둘을 조합해서, 훅이 필수인데 구현이 없으면
`static_assert`로 **컴파일 타임에** 실패한다 — `#include "ECS/ECSReflectionHook.h"`를
깜빡해서 컴포넌트가 조용히 미등록되는 실수를 그 자리에서 잡아낸다.

### 2.2 `MakeTypeFlags`/`MakePropertyMetadata` → `TypeFlagTrait`/`PropertyMetadataTrait`

같은 철학을 태그→플래그/메타데이터 변환에도 적용:

- `TypeFlagTrait<Tag>::VALUE` (타입 `ETypeFlags`) — Type 레벨 태그가 `TypeInfo::flags`에
  기여하는 값. Primary 템플릿은 `static_assert` 트랩이라, 특수화가 없는 태그를 쓰면
  컴파일 에러.
- `PropertyMetadataTrait<Tag>::Apply(PropertyMetadata&, const Tag&)` — Field 레벨 태그가
  `PropertyMetadata`(display_name/category/tooltip/flags/range)를 어떻게 채우는지. Range처럼
  태그 인스턴스의 값(min/max)이 필요한 경우가 있어서 타입이 아닌 인스턴스를 받는다.

`Reflect.h`의 `MakeTypeFlags`/`MakePropertyMetadata`는 이제 태그 이름을 전혀 모르는
fold-expression 호출만 남았다:

```cpp
auto process_tag = [&flags]<auto Tag>
{
    using TagType = std::remove_cvref_t<decltype(Tag)>;
    flags |= TypeFlagTrait<TagType>::VALUE;
};
```

### 2.3 반복 패턴 축소: 헬퍼 매크로 3개

트레이트 특수화가 15곳 이상으로 늘어나면서(그리고 앞으로 태그가 추가될 때마다 계속 늘어날
것이므로) 순수 스캐폴딩 반복이 눈에 띄기 시작했다. 값 하나만 다르고 나머지가 완전히 동일한
3가지 경우에만 매크로를 도입했다 (`TagTraits.h`):

| 매크로 | 대상 | 예시 |
|---|---|---|
| `SE_HOOK_REQUIRED(Tag)` | `HookRequiredTrait<Tag>::VALUE = true` | `SE_HOOK_REQUIRED(tags::Component);` |
| `SE_TYPE_FLAG(Tag, Flag)` | `TypeFlagTrait<Tag>::VALUE = Flag` | `SE_TYPE_FLAG(tags::Hidden, ETypeFlags::Hidden);` |
| `SE_PROPERTY_FLAG(Tag, Flag)` | `meta.flags \|= Flag`만 하는 `PropertyMetadataTrait<Tag>` | `SE_PROPERTY_FLAG(tags::ReadOnly, EPropertyFlags::ReadOnly);` |

`DisplayName`/`Category`/`Tooltip`/`Range`처럼 태그마다 본문 로직이 실제로 다른 경우(다른
필드에 대입, Range는 필드 2개 + 플래그 1개)는 매크로화하지 않고 그대로 코드로 남겼다 —
이유는 3장 참고.

---

## 3. 설계 결정과 기각된 대안

세션 중 여러 대안이 검토되고 기각됐다. 나중에 비슷한 확장점을 또 만들게 될 때 참고용으로 남긴다.

- **태그에 마커를 상속시키는 안 / 태그에 `RequiresHook` 같은 멤버를 직접 추가하는 안** — 기각.
  태그 자체(어디에 붙을 수 있는가를 나타내는 `target::Type`/`Field` 계층)에 무관한 관심사를
  섞는 꼴이 된다.
- **"필수 여부"와 "실제 구현"을 하나의 트레이트로 합치는 안** — 기각(언어 제약). "필수 여부"는
  태그 정의 시점(`Annotations.h`)에 항상 보여야 하고, "구현"은 소비자 헤더
  (`ECSReflectionHook.h`)를 include해야만 보인다. 이 둘을 같은 템플릿의 같은 특수화에
  넣으면, 두 헤더를 모두 include하는 정상 케이스에서 "struct 재정의" 컴파일 에러가 난다.
  즉 `HookRequiredTrait`/`RegistrationTrait`가 별개 템플릿인 건 스타일이 아니라 ODR 제약
  때문이다.
- **`Reflect.h` 안에 `same_as<TagType, tags::Component> || same_as<TagType, tags::Resource>`처럼
  하드코딩하는 안** — 기각. 새 훅 필요 태그가 생길 때마다 코어를 또 고쳐야 해서 애초에
  풀려던 문제로 되돌아간다.
- **런타임 검증(`Array<TypeId> applied_tags` + `ValidateRegistrations()`)** — 검토 후 폐기.
  "코어는 서술만 한다"는 원칙은 지키지만, 애플리케이션 실행 시점에야 실수를 잡는다. 최종적으로
  채택한 컴파일타임 `static_assert` 방식이 같은 문제를 더 일찍(컴파일 타임에) 잡아내므로
  런타임 안전망은 불필요해졌다.
- **`SE_REGISTRATION_HOOK(Type) { ... }` 같은 self-closing 매크로** — 처음엔 기각(사용처
  2곳뿐이라 "3번 반복 전 추상화 금지" 원칙에 위배). 이번 라운드에서 트레이트가 4종류·15곳
  이상으로 늘어나면서 재검토했고, **값 하나만 바뀌는 3가지 패턴에 한해서만** 매크로를
  도입했다. 본문 로직 자체가 다른 payload 계열(`DisplayName`/`Category`/`Tooltip`/`Range`)은
  매크로화해도 바깥 껍데기(3줄)만 줄고 가독성은 오히려 나빠져서 코드로 남겼다 — "매크로는
  스캐폴딩이 100% 동일할 때만" 이라는 기준으로 정리됨.
- **4개 트레이트 전체 리네이밍** — `HookRequired`/`RegistrationHook`(기존, 즉흥적으로 명명)과
  신규 두 트레이트의 이름 스타일이 어긋나 있어서, 이번에 4개 전부
  `HookRequiredTrait`/`RegistrationTrait`/`TypeFlagTrait`/`PropertyMetadataTrait`로 통일했다.
  규칙: 전부 `<Tag>`로 특수화, `...Trait`로 끝남, 상수면 `VALUE`, 동작이면 `Apply`.

---

## 4. AnnotationStore를 지금 도입하지 않은 이유

C++26 프로토타입 프로젝트(`cpp26_reflsystem`)에는 `AnnotationStore`(태그를 타입 소거해서
런타임에 `Has<T>()`/`Get<T>()`로 조회하는 범용 컨테이너)가 있다. `PropertyMetadata`의 고정
필드(`display_name`/`category`/`tooltip`/`range`)를 이걸로 대체하면 새 payload 태그를
추가해도 `PropertyMetadata` 구조체 자체를 건드릴 필요가 없어진다는 이점이 있다.

이번 리팩토링 범위에서는 제외했다:

- `AnnotationStore`의 핵심 가치는 `std::meta::annotations_of`로 소스 attribute를 자동
  수집하는 것인데, SimpleEngine은 아직 태그를 매크로에 수동으로 나열하므로 이 이점을 가져올
  수 없다 (C++26 없이는 "컨테이너를 범용화한다"는 절반의 이점만 남는다).
- 실사용 조사 결과 payload 태그(`DisplayName`/`Category`/`Tooltip`/`Range`) 사용처는 10개
  파일/26곳뿐이었고, `PropertyMetadata::category`는 현재 어디서도 읽지 않는 죽은 필드였다.
  아직 반복적 고통이 없는 문제에 type-erasure + NTTP 정적 저장소 같은 무거운 해법을 투입하는
  건 시기상조로 판단했다.
- `flags`(`ETypeFlags`/`EPropertyFlags`)는 에디터 인스펙터에서 매 프레임 체크되는 hot path이자
  애초에 boolean 집합이라는 표현 자체가 bitmask가 맞다. `AnnotationStore`가 도입되더라도
  flags는 계속 비트마스크로 유지하는 게 옳다고 판단했다(이번 트레이트들이 하는 일 자체가
  "태그를 한 번 평가해 bitmask로 캐싱"이므로, 원리는 미래에도 동일하게 유지된다).

---

## 5. 이후 할 일 / 알아둘 점

- **새 마커 태그를 추가할 때** (예: 미래의 `Deprecated` 태그): `Annotations.h`에 태그
  struct를 정의하고, 필요에 따라 `SE_TYPE_FLAG`/`SE_PROPERTY_FLAG`/`SE_HOOK_REQUIRED` 매크로
  한 줄만 추가하면 된다. `Reflect.h`는 건드릴 필요 없음.
- **새 payload 태그를 추가할 때** (예: 미래의 `Icon<Str>` 태그): `PropertyMetadata`에 필드를
  추가하고, `PropertyMetadataTrait<Tag>` 특수화(또는 기존 `*Base` 파생 패밀리에 편입)를
  작성한다. 이건 매크로화 대상이 아니므로 직접 코드로 작성.
- **새 소비자 훅을 추가할 때** (ECS 외에 또 다른 모듈이 태그 후처리가 필요해지면):
  `HookRequiredTrait<Tag>` 특수화(`Annotations.h`)와 `RegistrationTrait<Tag>` 특수화(소비자
  모듈의 헤더, `ECSReflectionHook.h`처럼)를 각각 작성.
- **C++26이 MSVC에 들어오면**: `Reflect.h` 상단의 `// TODO: C++26되면 여기 파일 전체 수정해야
  함` 주석대로, 태그를 모으는 방식(매크로의 수동 NTTP 팩) 자체가 `std::meta::annotations_of`
  기반 자동 열거로 바뀐다. 이건 지금 무엇을 하든 피할 수 없는 재작성이다. 다만 이번에 태그별로
  쪼개놓은 트레이트 특수화 본문(태그가 실제로 무엇을 의미하는지)은 수집 메커니즘이 바뀌어도
  거의 그대로 재사용 가능할 것으로 예상한다.
- **`AnnotationStore` 도입 트리거**: payload 태그 사용처가 크게 늘거나, `PropertyMetadata`에
  필드를 계속 추가하는 게 부담스러워지는 시점이 오면 재검토. 컨테이너/조회 메커니즘 자체는
  C++26 없이도(4장 참고) 지금 당장 구현 가능하다.
- 이 세션에서 실제로 겪은 버그(ECS 훅 헤더 include 누락 → 조용한 미등록)는 이제
  `static_assert`로 컴파일 타임에 잡힌다. 검증은 `Docs/Codex/00_Meta/QuickStart.md`의
  빌드 방법을 참고해 `EngineCore` 타겟과 `UnitTests`를 빌드해서 확인.
