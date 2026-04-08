// Gizmo 렌더링용 셰이더 (depth test OFF, 항상 씬 위에 렌더링)
// 정점 레이아웃: GizmoVertex (float3 position + float4 color)

#pragma se_shader vertex VSMain
#pragma se_shader fragment PSMain

cbuffer UBO : register(b0, space1)
{
    float4x4 VP;
}

struct VertexInput
{
    // C++: location 0 (Vector3f position, 월드 공간 좌표)
    float3 position : POSITION;

    // C++: location 1 (LinearColor color)
    float4 color : COLOR;
};

struct VertexOutput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

VertexOutput VSMain(VertexInput input)
{
    VertexOutput output;
    output.position = mul(VP, float4(input.position, 1.0f));
    output.color = input.color;
    return output;
}

float4 PSMain(VertexOutput input) : SV_TARGET
{
    return input.color;
}
