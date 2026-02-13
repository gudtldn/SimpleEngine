## TODO

- [ ] 각 Window를 기존의 렌더 방식을 RenderingPipeline으로 캡슐화
  - 이렇게 하면 Window별로 Rendering을 다르게할 수 있음
  - 윈도우별 DrawData 구현 (비슷한 내용)

- [ ] RenderGraph::Compile시 리소스 수명 체크해서 리소스 재사용 로직 추가
- [ ] RenderGraph에 Resource Extract 로직 추가
  - Extract를 하면 ResourcePool에서 std::unique_ptr<ExtractedResource>로 소유권을 넘겨서 반환
  - ExtractedResource가 소멸하면 다시 Pool로 반납
- [ ] 나중에 Thread-Safe해야하는 로직 확인하기
- [ ] ECS System클래스 구현
    - 생성자로 Fn&&을 받아서 시스템 끼리의 순서나 여러가지 상호작용을 할 수 있도록 하기
- [ ] RenderWorld 구현
    - 매 프레임 렌더링에 필요한 컴포넌트만 추출해서
- [ ] Entity를 Spawn할 때, 한번에 Entity + Components... 로 Spawn할 수 있도록 Bundle 기능 추가

- [ ] 프로파일러 만들기
    - CPU
    - GPU
    - Memory
    - Network
    - File I/O
    - Thread
    - 등등
- [ ] Log Category 만들기
- [ ] 메모리 할당자 개선
  - _expand나 realloc_in_place를 사용하여 개선
  - 아니면 나중에 제대로 OS로부터 Page를 받아서 메모리 Pool을 직접 구현
  - 디버그 빌드 시 가드 바이트(카나리)를 추가하여 메모리 손상 감지

- [ ] 지금 사용하고 있는 Assert의 출력을 AssertHandler로 리펙토링
  - `using AssertHandlerType = void(*)(const std::source_location& loc, std::string_view expr, std::string_view msg);`
  - 이런식으로 핸들러를 등록해서, Assert는 Dispatch만 하고
  - 핸들러를 등록할 때, 재귀를 방지하기 위해서 `thread_local bool is_asserting = false;`이런거 사용

- [ ] 나중에 I18n을 지원할 때, ICU4X도 한번 검토하기
  - ICU4X를 링크하려면 rust compiler가 필요한데, 엔진에서 진짜 필요한지 봐야할 듯.

## 완료

- [x] 모듈명, 넴스 이름 규칙 정하고 다시 점검하기
- [x] StringName(FName) 구현
- [x] Log기능 source_location을 이용해서 만들기
- [x] Log에 담기는 정보를 구조체로 빼서 조합 (source_location, message, timestamp 등)
- [x] ISubsystem에 GetSubsystem을 추가해서 Dependency검사후 주는 방식으로 하는것도 나쁘지 않을듯
- [x] Log Backend 만들기
    - Log On/Off 가능하게
    - Log도 Subsystem으로 할까
- [x] 엔진 구조 개선하기
    - [x] 엔진 개선 참고 1
    - [x] 엔진 개선 참고 2
    - [x] namespace 모두 부착
    - [x] 다중 윈도우 지원
- [x] 커스텀 메모리 할당자 구현
- [x] RenderGraph 구현
    - GPU Resource Pool 구현
    - 이후 Realize() 로직을 Pool에서 가져오는 걸로 변경
- [x] 가상 경로 시스템 구현
- [x] ECS World에 RegisterSystem 구현
    - 이래야 멀티 월드일 때 구성하기가 쉬움
- [x] 컴파일 타임에 알 수 있는 `std::type_index`를 TypeId로 변경
- [x] cmake로 크로스 플랫폼 빌드 구성하기
- [x] 나중에 SDL3를 submodule로 추가하기 (vcpkg로 관리)
- [x] 나중에 icu4c를 submodule로 추가하기 (vcpkg로 관리)
- [x] Optional constexpr로 만들기
  - 주석 다시한번 확인하고, 함수이름과 requires절 가독성 개선
- [x] InputSubsystemSystem 구현
- [x] PlatformEventDispatcher 리펙토링
