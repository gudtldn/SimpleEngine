// Gizmo Pick Shader

#pragma se_shader vertex VSMain
#pragma se_shader fragment PSMain

cbuffer UBO : register(b0, space1)
{
    float4x4 VP;
    float3 GizmoCenter; // 기즈모 월드 중심 좌표
    float ScreenScale;  // 원근 보정 스케일 (per-viewport)
}

struct VertexInput
{
    // C++: location 0 (Vector3f position, 기즈모 로컬 공간)
    float3 position : POSITION;

    // C++: location 1 (LinearColor color)
    float4 color : COLOR; // Gizmo Pick Shader에서는 무시

    // C++: location 2 (Pick ID)
    uint pick_id : TEXCOORD0;
};

struct VertexOutput
{
    float4 position : SV_POSITION;
    nointerpolation uint pick_id : TEXCOORD0;
};

VertexOutput VSMain(VertexInput input)
{
    VertexOutput output;
    float3 world_pos = GizmoCenter + input.position * ScreenScale;
    output.position = mul(VP, float4(world_pos, 1.0f));
    output.pick_id = input.pick_id;
    return output;
}

uint PSMain(VertexOutput input) : SV_TARGET
{
    return input.pick_id;
}
