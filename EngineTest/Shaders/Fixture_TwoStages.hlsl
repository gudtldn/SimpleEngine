// 정점·픽셀 스테이지가 각자 다른 set의 리소스를 씁니다.

cbuffer PassUBO : register(b0, space1)
{
    float4x4 view_projection;
}

cbuffer ObjectUBO : register(b1, space1)
{
    float4x4 model;
    uint entity_id;
}

Texture2D base_color_texture : register(t0, space2);
SamplerState base_color_sampler : register(s0, space2);

cbuffer MaterialUBO : register(b0, space3)
{
    float4 base_color_factor;
}

struct VertexInput
{
    [[vk::location(0)]] float3 position  : TEXCOORD0;
    [[vk::location(1)]] float2 tex_coord : TEXCOORD1;
};

struct VertexOutput
{
    float4 position : SV_Position;
    float2 tex_coord : TEXCOORD0;
    nointerpolation uint entity_id : TEXCOORD1;
};

[shader("vertex")]
VertexOutput VSMain(VertexInput input)
{
    VertexOutput output;
    output.position = mul(view_projection, mul(model, float4(input.position, 1.0f)));
    output.tex_coord = input.tex_coord;
    output.entity_id = entity_id;
    return output;
}

[shader("pixel")]
float4 PSMain(VertexOutput input) : SV_Target0
{
    return base_color_texture.Sample(base_color_sampler, input.tex_coord) * base_color_factor;
}
