## TODO

- [ ] Optional constexpr로 만들기
- [ ] RenderGraph::Compile시 리소스 수명 체크해서 리소스 재사용 로직 추가
- [ ] 나중에 Thread-Safe해야하는 로직 확인하기
- [ ] 윈도우별 DrawData 구현
- [ ] ECS System클래스 구현
  - 생성자로 Fn&&을 받아서 시스템 끼리의 순서나 여러가지 상호작용을 할 수 있도록 하기
- [ ] RenderWorld 구현
  - 매 프레임 렌더링에 필요한 컴포넌트만 추출해서
- [ ] InputSystem 구현

- [ ] 프로파일러 만들기
  - CPU
  - GPU
  - Memory
  - Network
  - File I/O
  - Thread
  - 등등
- [ ] Log Category 만들기
- [ ] cmake로 크로스 플랫폼 빌드 구성하기
- [ ] 나중에 SDL3를 submodule로 추가하기
- [ ] 나중에 icu4c를 submodule로 추가하기

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
