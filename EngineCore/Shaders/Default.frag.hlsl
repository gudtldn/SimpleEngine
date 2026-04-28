// FS에서 Uniform Buffer의 space 설정
// https://wiki.libsdl.org/SDL3/SDL_CreateGPUShader#remarks

#pragma se_shader fragment PSMain

#include "Default.hlsli"

// per-material (머티리얼 인스턴스 교체 시 변경)
cbuffer MaterialUBO : register(b0, space3)
{
    float4 BaseColor;  // 기본 색상 (RGBA)
    float AlphaCutoff; // 투명도 컷오프 (알파 테스팅 용도)
    uint Flags;        // 재질 속성 플래그 (비트마스크 처리용)
}

// t0: Texture(이미지 데이터 배열), s0: Sampler(이미지를 어떻게 읽을지 결정하는 필터링/래핑 설정)
Texture2D<float4> BaseColorTexture : register(t0, space2);
SamplerState BaseColorSampler : register(s0, space2);

// MRT (Multiple Render Targets) 레이아웃
struct PSOutput
{
    float4 color : SV_Target0;   // RT0: 씬 컬러 버퍼
    uint entity_id : SV_Target1; // RT1: 엔티티 ID 버퍼
};

PSOutput PSMain(VertexOutput input)
{
    PSOutput output;

    output.color = BaseColorTexture.Sample(BaseColorSampler, input.tex_coord) * BaseColor;
    output.entity_id = input.entity_id;

    return output;
}
