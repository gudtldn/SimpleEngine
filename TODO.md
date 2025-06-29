## TODO

- [ ] 엔진 구조 개선하기
  - [ ] [엔진 개선 참고 1](https://www.perplexity.ai/search/naega-jigeum-sdl3wa-sdl3gpu-im-_DxIE29lTLq4VdhMutaWcA)
  - [ ] [엔진 개선 참고 2](https://aistudio.google.com/prompts/1BRsDCWohn6GONAaPJK88NiCAT-xJOCuW?_gl=1*1xqpzh6*_up*MQ..*_ga*MTY4OTQwMjM1NC4xNzQ5NTQ0MjU3*_ga_RJSPDF5Y0Q*czE3NDk1NTQ0MjQkbzIkZzAkdDE3NDk1NTQ0MjQkajYwJGwwJGgw*_ga_P1DBVKWT6V*czE3NDk1NTQ0MjQkbzIkZzAkdDE3NDk1NTQ0MjQkajYwJGwwJGgxNTU1NTk0MjI4)
  - [ ] namespace 모두 부착
  - [ ] 다중 윈도우 지원

- [ ] Log Backend 만들기
  - Log On/Off 가능하게
- [ ] Log Category 만들기
- [ ] Log도 Subsystem으로 할까
- [ ] ISubsystem에 GetSubsystem을 추가해서 Dependency검사후 주는 방식으로 하는것도 나쁘지 않을듯
- [ ] cmake로 크로스 플랫폼 빌드 구성하기
- [ ] 나중에 SDL3를 submodule로 추가하기

## 완료
- [x] Log기능 source_location을 이용해서 만들기
- [x] Log에 담기는 정보를 구조체로 빼서 조합 (source_location, message, timestamp 등)
