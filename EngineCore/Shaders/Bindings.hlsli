// SDL3 GPU가 정한 스테이지별 리소스 위치를 이름으로 감쌉니다.
// https://wiki.libsdl.org/SDL3/SDL_CreateGPUShader#remarks
#ifndef SE_BINDINGS_HLSLI
#define SE_BINDINGS_HLSLI

#define SE_CONCAT_IMPL(a, b) a##b
#define SE_CONCAT(a, b) SE_CONCAT_IMPL(a, b)

// 정점 셰이더 상수 버퍼는 space1
#define SE_VS_UNIFORM(slot) register(SE_CONCAT(b, slot), space1)

// 픽셀 셰이더 텍스처/샘플러는 space2, 상수 버퍼는 space3
#define SE_PS_TEXTURE(slot) register(SE_CONCAT(t, slot), space2)
#define SE_PS_SAMPLER(slot) register(SE_CONCAT(s, slot), space2)
#define SE_PS_UNIFORM(slot) register(SE_CONCAT(b, slot), space3)

// 상수 버퍼 슬롯은 갱신 빈도로 설정
#define SE_SLOT_PASS        0 // 패스마다
#define SE_VS_SLOT_OBJECT   1 // 드로우마다
#define SE_PS_SLOT_MATERIAL 1 // 머티리얼마다

#endif
