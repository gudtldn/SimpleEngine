// D3D 패킹 규칙이 드러나는 상수 버퍼 레이아웃입니다.

cbuffer LayoutUBO : register(b0, space3)
{
    float a;
    float2 b;
    float c;
    float3 d;
    float e[2];
    float f;
    float4x4 g;
    uint h;
}

[shader("vertex")]
float4 VSMain(uint vertex_id : SV_VertexID) : SV_Position
{
    return float4(float(vertex_id), 0.0f, 0.0f, 1.0f);
}

[shader("pixel")]
float4 PSMain() : SV_Target0
{
    return float4(a + b.x + c, d.x + e[0] + e[1], f + g[0][0], float(h));
}
