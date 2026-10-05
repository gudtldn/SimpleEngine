// 텍스처 t0과 t4만 선언해 슬롯 1~3이 빕니다.

Texture2D first_texture : register(t0, space2);
Texture2D fifth_texture : register(t4, space2);
SamplerState shared_sampler : register(s0, space2);

struct VertexOutput
{
    float4 position : SV_Position;
    float2 tex_coord : TEXCOORD0;
};

[shader("vertex")]
VertexOutput VSMain(uint vertex_id : SV_VertexID)
{
    VertexOutput output;
    output.tex_coord = float2((vertex_id << 1) & 2, vertex_id & 2);
    output.position = float4(output.tex_coord * 2.0f - 1.0f, 0.0f, 1.0f);
    return output;
}

[shader("pixel")]
float4 PSMain(VertexOutput input) : SV_Target0
{
    return first_texture.Sample(shared_sampler, input.tex_coord) + fifth_texture.Sample(shared_sampler, input.tex_coord);
}
