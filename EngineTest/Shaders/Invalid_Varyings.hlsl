// 픽셀 입력이 정점 출력과 타입, location, semantic에서 각각 하나씩 어긋납니다.

struct VertexOutput
{
    float4 position : SV_Position;
    float2 tex_coord : TEXCOORD0;
    float3 world_normal : TEXCOORD1;
    float4 color : TEXCOORD2;
};

struct PixelInput
{
    float4 position : SV_Position;
    float3 tex_coord : TEXCOORD0;
    float4 color : TEXCOORD2;
    float3 tangent : TEXCOORD3;
};

[shader("vertex")]
VertexOutput VSMain(uint vertex_id : SV_VertexID)
{
    VertexOutput output;
    output.position = float4(float(vertex_id), 0.0f, 0.0f, 1.0f);
    output.tex_coord = float2(0.0f, 0.0f);
    output.world_normal = float3(0.0f, 0.0f, 1.0f);
    output.color = float4(1.0f, 1.0f, 1.0f, 1.0f);
    return output;
}

[shader("pixel")]
float4 PSMain(PixelInput input) : SV_Target0
{
    return float4(input.tex_coord + input.tangent, 1.0f) * input.color;
}
