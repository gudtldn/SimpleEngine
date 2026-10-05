// Gizmo Pick Shader

#pragma se_shader vertex VSMain
#pragma se_shader fragment PSMain

#include "Bindings.hlsli"

cbuffer UBO : SE_VS_UNIFORM(SE_SLOT_PASS)
{
    float4x4 vp;
    float3 gizmo_center; // 기즈모 월드 중심 좌표
    float screen_scale;  // 원근 보정 스케일 (per-viewport)
}

struct VertexInput
{
    // C++: Vector3f position (기즈모 로컬 공간)
    [[vk::location(0)]] float3 position : TEXCOORD0;

    // C++: LinearColor color
    [[vk::location(1)]] float4 color : TEXCOORD1; // Gizmo Pick Shader에서는 무시

    // C++: Pick ID
    [[vk::location(2)]] uint pick_id : TEXCOORD2;
};

struct VertexOutput
{
    float4 position : SV_POSITION;
    nointerpolation uint pick_id : TEXCOORD0;
};

VertexOutput VSMain(VertexInput input)
{
    VertexOutput output;
    float3 world_pos = gizmo_center + input.position * screen_scale;
    output.position = mul(vp, float4(world_pos, 1.0f));
    output.pick_id = input.pick_id;
    return output;
}

uint PSMain(VertexOutput input) : SV_TARGET
{
    return input.pick_id;
}
