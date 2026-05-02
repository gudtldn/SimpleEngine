// FS에서 Uniform Buffer의 space 설정
// https://wiki.libsdl.org/SDL3/SDL_CreateGPUShader#remarks

#pragma se_shader fragment PSMain

#include "Default.hlsli"

// per-material (머티리얼 인스턴스 교체 시 변경)
// std140 레이아웃: BaseColor(16B) + AlphaCutoff(4B) + Flags(4B) + _pad(8B) = 32B
cbuffer MaterialUBO : register(b0, space3)
{
    float4 BaseColor;  // 기본 색상 (RGBA)
    float AlphaCutoff; // 투명도 컷오프 (알파 테스팅 용도)
    uint Flags;        // 재질 속성 플래그 (비트마스크, bit0 = alpha test 활성화)
    float2 _pad;
}

// t0: Texture(이미지 데이터), s0: Sampler(필터링/래핑 설정)
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

    float4 result = BaseColorTexture.Sample(BaseColorSampler, input.tex_coord) * BaseColor;

    // bit 0: alpha test 활성화
    if ((Flags & 0x1u) != 0u && result.a < AlphaCutoff)
    {
        discard;
    }

    output.color = result;
    output.entity_id = input.entity_id;

    return output;
}