// 첫 정점 입력이 TEXCOORD0 대신 POSITION semantic을 씁니다.

struct VertexInput
{
    float3 position : POSITION;
    float2 tex_coord : TEXCOORD1;
};

[shader("vertex")]
float4 VSMain(VertexInput input) : SV_Position
{
    return float4(input.position + float3(input.tex_coord, 0.0f), 1.0f);
}
