// Fixture_Include.hlsl이 포함하는 헤더입니다.
#ifndef FIXTURE_COMMON_HLSLI
#define FIXTURE_COMMON_HLSLI

float4 FullscreenPosition(uint vertex_id)
{
    const float2 uv = float2((vertex_id << 1) & 2, vertex_id & 2);
    return float4(uv * 2.0f - 1.0f, 0.0f, 1.0f);
}

#endif
