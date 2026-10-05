#pragma se_shader vertex VSMain
#pragma se_shader fragment PSMain

#include "Bindings.hlsli"

cbuffer UBO : SE_VS_UNIFORM(SE_SLOT_PASS)
{
    float4x4 vp;
}

struct VertexInput
{
    // C++: Vector3f position (월드 공간 좌표)
    [[vk::location(0)]] float3 position : TEXCOORD0;

    // C++: LinearColor color
    [[vk::location(1)]] float4 color : TEXCOORD1;
};

struct VertexOutput
{
    float4 position : SV_POSITION;
    float4 color : TEXCOORD0;
};

VertexOutput VSMain(VertexInput input)
{
    VertexOutput output;
    output.position = mul(vp, float4(input.position, 1.0f));
    output.color = input.color;
    return output;
}

float4 PSMain(VertexOutput input) : SV_TARGET
{
    return input.color;
}
